import { createHash } from "node:crypto";
import path from "node:path";
import { buildReaderPackageFromChapters, serializeReaderPackage } from "./reader-package.js";
import { decodeHtmlEntities, normalizeParagraphs } from "./unicode.js";
import { ZipArchive } from "./zip.js";

const XML_NS = /<\?xml[\s\S]*?\?>/giu;
const SKIPPED_HTML = /<(head|script|style|svg|math|nav)\b[\s\S]*?<\/\1>/giu;
const HTML_COMMENT = /<!--[\s\S]*?-->/gu;
const HEADING = /<h[1-6]\b[^>]*>([\s\S]*?)<\/h[1-6]>/giu;
const BLOCK_OPEN = /<(p|div|section|article|blockquote|figcaption|li|tr|td|th|br|hr)\b[^>]*\/?>/giu;
const BLOCK_CLOSE = /<\/(p|div|section|article|blockquote|figcaption|li|tr|td|th)>/giu;
const TAG = /<[^>]+>/gu;
const COVER_FILE = /(^|[-_/\s.])cover([-_\s.]|$)/iu;
const IMAGE_TYPES_BY_EXTENSION = new Map([
  [".gif", "image/gif"],
  [".jpeg", "image/jpeg"],
  [".jpg", "image/jpeg"],
  [".png", "image/png"],
  [".webp", "image/webp"]
]);
const IMAGE_TYPES = new Set(IMAGE_TYPES_BY_EXTENSION.values());

function sha256(bytes) {
  return createHash("sha256").update(bytes).digest("hex");
}

function attributesFromTag(tag) {
  const attributes = {};
  for (const match of tag.matchAll(/([:\w-]+)\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+))/gu)) {
    attributes[match[1].toLowerCase()] = decodeHtmlEntities(match[2] ?? match[3] ?? match[4] ?? "");
  }
  return attributes;
}

function stripTags(fragment) {
  return decodeHtmlEntities(String(fragment).replace(TAG, " "))
    .replace(/\s+/gu, " ")
    .trim()
    .normalize("NFC");
}

function chapterMarkerForHeading(heading) {
  const embedded = String(heading).match(/@@PEBBLE_RSVP_CHAPTER:([\s\S]*?)@@/u);
  const title = embedded?.[1]?.trim() || stripTags(heading);
  return `\n\n@@PEBBLE_RSVP_CHAPTER:${title}@@\n\n`;
}

function firstXmlText(xml, localName) {
  const pattern = new RegExp(`<(?:[\\w-]+:)?${localName}\\b[^>]*>([\\s\\S]*?)<\\/(?:[\\w-]+:)?${localName}>`, "iu");
  const match = pattern.exec(xml);
  return match ? stripTags(match[1]) : "";
}

function allXmlText(xml, localName) {
  const pattern = new RegExp(`<(?:[\\w-]+:)?${localName}\\b[^>]*>([\\s\\S]*?)<\\/(?:[\\w-]+:)?${localName}>`, "giu");
  return Array.from(xml.matchAll(pattern), (match) => stripTags(match[1])).filter(Boolean);
}

function resolveZipPath(baseDirectory, href) {
  const withoutFragment = String(href).split("#")[0].split("?")[0];
  const decoded = decodeURIComponent(withoutFragment).replace(/\\/gu, "/");
  return path.posix.normalize(path.posix.join(baseDirectory, decoded)).replace(/^\/+/u, "");
}

function zipDirectory(zipPath) {
  const directory = path.posix.dirname(zipPath);
  return directory === "." ? "" : `${directory}/`;
}

function fileTitle(zipPath) {
  const base = path.posix.basename(zipPath).replace(/\.[^.]+$/u, "");
  return decodeURIComponent(base).replace(/[_-]+/gu, " ").replace(/\s+/gu, " ").trim() || "Chapter";
}

function extractOpfPath(containerXml) {
  const rootfileMatch = containerXml.match(/<rootfile\b[^>]*>/iu);
  if (!rootfileMatch) {
    throw new Error("EPUB container.xml does not contain a rootfile");
  }
  const attributes = attributesFromTag(rootfileMatch[0]);
  if (!attributes["full-path"]) {
    throw new Error("EPUB rootfile is missing full-path");
  }
  return attributes["full-path"].replace(/^\/+/u, "");
}

function parseManifest(opfXml, opfDirectory) {
  const manifest = new Map();
  for (const match of opfXml.matchAll(/<item\b[^>]*>/giu)) {
    const attributes = attributesFromTag(match[0]);
    if (!attributes.id || !attributes.href) {
      continue;
    }
    manifest.set(attributes.id, {
      id: attributes.id,
      href: attributes.href,
      path: resolveZipPath(opfDirectory, attributes.href),
      mediaType: attributes["media-type"] ?? "",
      properties: attributes.properties ?? ""
    });
  }
  return manifest;
}

function parseSpine(opfXml) {
  const spineMatch = opfXml.match(/<spine\b[\s\S]*?<\/spine>/iu);
  if (!spineMatch) {
    return [];
  }
  return Array.from(spineMatch[0].matchAll(/<itemref\b[^>]*>/giu), (match) => attributesFromTag(match[0]).idref).filter(Boolean);
}

function parseSpineTocId(opfXml) {
  const spineTag = opfXml.match(/<spine\b[^>]*>/iu);
  return spineTag ? attributesFromTag(spineTag[0]).toc ?? "" : "";
}

function escapeRegExp(value) {
  return String(value).replace(/[.*+?^${}()|[\]\\]/gu, "\\$&");
}

function parseNcxEntries(xml, ncxPath) {
  const entries = [];
  const pattern = /<navPoint\b[\s\S]*?<navLabel\b[\s\S]*?<text\b[^>]*>([\s\S]*?)<\/text>[\s\S]*?<content\b[^>]*>/giu;
  for (const match of xml.matchAll(pattern)) {
    const contentTag = match[0].match(/<content\b[^>]*>/iu)?.[0];
    const src = contentTag ? attributesFromTag(contentTag).src : "";
    const title = stripTags(match[1]);
    if (!src || !title) continue;
    const [href, fragment = ""] = src.split("#", 2);
    entries.push({
      path: resolveZipPath(zipDirectory(ncxPath), href),
      fragment: decodeURIComponent(fragment),
      title
    });
  }
  return entries;
}

function parseNavEntries(xml, navPath) {
  const entries = [];
  for (const match of xml.matchAll(/<a\b[^>]*href\s*=\s*(?:"([^"]+)"|'([^']+)')[^>]*>([\s\S]*?)<\/a>/giu)) {
    const hrefValue = decodeHtmlEntities(match[1] ?? match[2] ?? "");
    const title = stripTags(match[3]);
    if (!hrefValue || !title) continue;
    const [href, fragment = ""] = hrefValue.split("#", 2);
    entries.push({
      path: resolveZipPath(zipDirectory(navPath), href),
      fragment: decodeURIComponent(fragment),
      title
    });
  }
  return entries;
}

function extractNavigationEntries(zip, manifest, opfXml) {
  const tocId = parseSpineTocId(opfXml);
  const ncxItem = (tocId && manifest.get(tocId)) ||
    Array.from(manifest.values()).find((item) => item.mediaType.toLowerCase() === "application/x-dtbncx+xml");
  if (ncxItem) {
    try {
      const entries = parseNcxEntries(zip.readTextEntry(ncxItem.path), ncxItem.path);
      if (entries.length > 0) return entries;
    } catch {
      // A malformed navigation file must not make otherwise readable content unusable.
    }
  }
  const navItem = Array.from(manifest.values()).find((item) => propertyTokens(item).includes("nav"));
  if (navItem) {
    try {
      return parseNavEntries(zip.readTextEntry(navItem.path), navItem.path);
    } catch {
      return [];
    }
  }
  return [];
}

function parseCoverId(opfXml) {
  for (const match of opfXml.matchAll(/<meta\b[^>]*>/giu)) {
    const attributes = attributesFromTag(match[0]);
    if (attributes.name?.toLowerCase() === "cover" && attributes.content) {
      return attributes.content;
    }
  }
  return "";
}

function parseGuideCoverPaths(opfXml, opfDirectory) {
  const guideMatch = opfXml.match(/<guide\b[\s\S]*?<\/guide>/iu);
  if (!guideMatch) {
    return [];
  }

  const paths = [];
  for (const match of guideMatch[0].matchAll(/<reference\b[^>]*>/giu)) {
    const attributes = attributesFromTag(match[0]);
    if (attributes.type?.toLowerCase() === "cover" && attributes.href) {
      paths.push(resolveZipPath(opfDirectory, attributes.href));
    }
  }
  return paths;
}

function imageContentTypeForPath(zipPath) {
  return IMAGE_TYPES_BY_EXTENSION.get(path.posix.extname(zipPath).toLowerCase()) ?? "";
}

function inferImageContentType(item) {
  const mediaType = item.mediaType.toLowerCase();
  if (mediaType === "image/jpg") {
    return "image/jpeg";
  }
  if (IMAGE_TYPES.has(mediaType)) {
    return mediaType;
  }
  return imageContentTypeForPath(item.path);
}

function propertyTokens(item) {
  return item.properties.toLowerCase().split(/\s+/u).filter(Boolean);
}

function readCoverPath(zip, zipPath, source) {
  const contentType = imageContentTypeForPath(zipPath);
  if (!contentType) {
    return null;
  }
  try {
    return {
      bytes: zip.readEntry(zipPath),
      contentType,
      path: zipPath,
      source
    };
  } catch {
    return null;
  }
}

function imageReferencesFromDocument(html, documentPath) {
  const references = [];
  for (const match of html.matchAll(/<(?:img|image)\b[^>]*>/giu)) {
    const attributes = attributesFromTag(match[0]);
    const href = attributes.src ?? attributes.href ?? attributes["xlink:href"] ?? "";
    if (href && !href.toLowerCase().startsWith("data:")) {
      references.push(resolveZipPath(zipDirectory(documentPath), href));
    }
  }
  return references;
}

function extractGuideCover(zip, guideCoverPaths) {
  for (const guideCoverPath of guideCoverPaths) {
    const directCover = readCoverPath(zip, guideCoverPath, "guide-reference");
    if (directCover) {
      return directCover;
    }

    let coverDocument = "";
    try {
      coverDocument = zip.readTextEntry(guideCoverPath);
    } catch {
      continue;
    }

    for (const imagePath of imageReferencesFromDocument(coverDocument, guideCoverPath)) {
      const cover = readCoverPath(zip, imagePath, "guide-page");
      if (cover) {
        return cover;
      }
    }
  }
  return null;
}

function extractCover(zip, manifest, coverId, guideCoverPaths) {
  const passes = [
    {
      source: "manifest-properties",
      matches: (item) => propertyTokens(item).includes("cover-image")
    },
    {
      source: "metadata-cover",
      matches: (item) => coverId && item.id === coverId
    }
  ];

  for (const pass of passes) {
    for (const item of manifest.values()) {
      const contentType = inferImageContentType(item);
      if (!pass.matches(item) || !contentType) {
        continue;
      }
      try {
        return {
          bytes: zip.readEntry(item.path),
          contentType,
          path: item.path,
          source: pass.source
        };
      } catch {
        continue;
      }
    }
  }

  const guideCover = extractGuideCover(zip, guideCoverPaths);
  if (guideCover) {
    return guideCover;
  }

  for (const item of manifest.values()) {
    const contentType = inferImageContentType(item);
    if (!COVER_FILE.test(path.posix.basename(item.path)) || !contentType) {
      continue;
    }
    try {
      return {
        bytes: zip.readEntry(item.path),
        contentType,
        path: item.path,
        source: "filename"
      };
    } catch {
      continue;
    }
  }
  return null;
}

function readEpubPackage(bytes) {
  const source = Buffer.from(bytes);
  const zip = new ZipArchive(source);
  if (zip.hasEntry("META-INF/encryption.xml")) {
    throw new Error("encrypted EPUBs are not supported");
  }
  if (!zip.hasEntry("META-INF/container.xml")) {
    throw new Error("EPUB is missing META-INF/container.xml");
  }

  const opfPath = extractOpfPath(zip.readTextEntry("META-INF/container.xml"));
  const opfDirectory = zipDirectory(opfPath);
  const opfXml = zip.readTextEntry(opfPath);
  const manifest = parseManifest(opfXml, opfDirectory);
  const coverId = parseCoverId(opfXml);
  const guideCoverPaths = parseGuideCoverPaths(opfXml, opfDirectory);

  return {
    source,
    zip,
    opfPath,
    opfXml,
    manifest,
    coverId,
    guideCoverPaths
  };
}

export function extractEpubCover(bytes) {
  const { zip, manifest, coverId, guideCoverPaths } = readEpubPackage(bytes);
  return extractCover(zip, manifest, coverId, guideCoverPaths);
}

function isContentItem(item) {
  const mediaType = item.mediaType.toLowerCase();
  const loweredPath = item.path.toLowerCase();
  return (
    mediaType === "application/xhtml+xml" ||
    mediaType === "text/html" ||
    loweredPath.endsWith(".xhtml") ||
    loweredPath.endsWith(".html") ||
    loweredPath.endsWith(".htm")
  );
}

function xhtmlToChapters(html, fallbackTitle, navigationEntries = []) {
  let structured = String(html);
  for (const entry of navigationEntries) {
    if (!entry.fragment) continue;
    const marker = `\n\n@@PEBBLE_RSVP_CHAPTER:${entry.title}@@\n\n`;
    const idPattern = new RegExp(`(<[a-z][^>]*\\bid\\s*=\\s*["']${escapeRegExp(entry.fragment)}["'][^>]*>)`, "iu");
    structured = structured.replace(idPattern, `${marker}$1`);
  }
  const cleaned = structured
    .replace(XML_NS, "")
    .replace(HTML_COMMENT, "")
    .replace(SKIPPED_HTML, "")
    .replace(HEADING, (_match, heading) => chapterMarkerForHeading(heading))
    .replace(BLOCK_OPEN, "\n\n")
    .replace(BLOCK_CLOSE, "\n\n")
    // Inline markup does not create whitespace in HTML. In particular, publishers
    // frequently change emphasis in the middle of a word (d<span>ie</span>).
    .replace(TAG, "");

  const chunks = cleaned.split(/\n\s*\n/gu);
  const chapters = [];
  let current = {
    title: fallbackTitle,
    paragraphs: []
  };

  for (const chunk of chunks) {
    const trimmed = chunk.trim();
    if (!trimmed) {
      continue;
    }
    if (trimmed.startsWith("@@PEBBLE_RSVP_CHAPTER:")) {
      if (current.paragraphs.length > 0) {
        chapters.push(current);
      }
      const markerEnd = trimmed.indexOf("@@", "@@PEBBLE_RSVP_CHAPTER:".length);
      const markerTitle = markerEnd >= 0
        ? trimmed.slice("@@PEBBLE_RSVP_CHAPTER:".length, markerEnd).trim()
        : trimmed.slice("@@PEBBLE_RSVP_CHAPTER:".length).trim();
      current = {
        title: markerTitle || fallbackTitle,
        paragraphs: []
      };
      if (markerEnd >= 0) {
        current.paragraphs.push(...normalizeParagraphs(trimmed.slice(markerEnd + 2).trim()));
      }
      continue;
    }

    const paragraphs = normalizeParagraphs(trimmed);
    current.paragraphs.push(...paragraphs);
  }

  if (current.paragraphs.length > 0) {
    chapters.push(current);
  }
  return chapters.length > 0 ? chapters : [];
}

const GENERATED_CHAPTER_TITLE = /^(?:(?:part|content|text|chapter|chap|kapitel|section|page)\s*0*\d+(?:[.:]\d+)?|0\d{2,}|\d{1,2}:\d{1,2}|\d{8,}(?:\s+\d+(?:\.\d+)?)?|\d+\s+(?:vita|lovelybooks\w*|impressum))$/iu;
const BACK_MATTER_PATTERNS = [
  [/^(?:danksagung|acknowledg(?:e)?ments?)\b/iu, ["Acknowledgements", "Danksagung"]],
  [/^(?:(?:ueber|über)\s+(?:den|die)\s+(?:autor|autorin|autoren)|about\s+the\s+author|vita\b)/iu,
    ["About the author", "Über den Autor"]],
  [/^(?:(?:ueber|über)\s+(?:den|die)\s+(?:uebersetzer|übersetzer|uebersetzerin|übersetzerin)|translation\b)/iu,
    ["Translation", "Übersetzung"]],
  [/^(?:impressum|copyright)|\bimpressum$/iu, ["Imprint", "Impressum"]],
  [/^(?:newsletter|weitere\s+(?:buecher|bücher)|also\s+by)|lovelybooks/iu, ["More books", "Weitere Bücher"]],
  [/^(?:inhalt|contents?)\b/iu, ["Contents", "Inhalt"]]
];

function chapterWordCount(chapter) {
  return chapter.paragraphs.join(" ").trim().split(/\s+/u).filter(Boolean).length;
}

function semanticGeneratedTitle(chapter, language) {
  const sample = chapter.paragraphs.slice(0, 4).join(" ").slice(0, 700);
  for (const [pattern, titles] of BACK_MATTER_PATTERNS) {
    if (pattern.test(sample)) {
      return language.toLowerCase().startsWith("de") ? titles[1] : titles[0];
    }
  }
  return "";
}

/**
 * Replace EPUB implementation filenames with stable reader-facing labels. Numeric
 * headings authored by the publisher are deliberately preserved.
 */
export function normalizeEpubChapterTitles(chapters, language = "und") {
  const result = chapters.map((chapter) => ({ ...chapter, paragraphs: [...chapter.paragraphs] }));
  const authoredNumeric = result
    .map((chapter, index) => (/^(?:0|[1-9]\d*)$/u.test(chapter.title) ? index : -1))
    .filter((index) => index >= 0);
  if (authoredNumeric.length > 0) {
    const lastNumeric = authoredNumeric.at(-1);
    for (let index = 0; index < result.length; index += 1) {
      const chapter = result[index];
      if (!GENERATED_CHAPTER_TITLE.test(chapter.title)) continue;
      const semantic = index > lastNumeric ? semanticGeneratedTitle(chapter, language) : "";
      chapter.title = semantic || (index > lastNumeric
        ? (language.toLowerCase().startsWith("de") ? "Anhang" : "Appendix")
        : (language.toLowerCase().startsWith("de") ? "Vorspann" : "Front matter"));
    }
    return result.map((chapter) => ({
      ...chapter,
      showHeading: !/^(?:front matter|vorspann|appendix|anhang)$/iu.test(chapter.title)
    }));
  }
  let firstBody = result.findIndex((chapter) =>
    GENERATED_CHAPTER_TITLE.test(chapter.title) && chapterWordCount(chapter) >= 120
  );
  if (firstBody < 0) {
    firstBody = result.findIndex((chapter) => GENERATED_CHAPTER_TITLE.test(chapter.title));
  }
  if (firstBody < 0) {
    return result.map((chapter) => ({ ...chapter, showHeading: true }));
  }

  let lastBody = -1;
  for (let index = firstBody; index < result.length; index += 1) {
    if (GENERATED_CHAPTER_TITLE.test(result[index].title) && chapterWordCount(result[index]) >= 500) {
      lastBody = index;
    }
  }
  if (lastBody < firstBody) {
    for (let index = firstBody; index < result.length; index += 1) {
      if (GENERATED_CHAPTER_TITLE.test(result[index].title) && chapterWordCount(result[index]) >= 120) {
        lastBody = index;
      }
    }
  }

  let chapterNumber = 1;
  let backMatterStarted = false;
  for (let index = 0; index < result.length; index += 1) {
    const chapter = result[index];
    if (!GENERATED_CHAPTER_TITLE.test(chapter.title)) {
      continue;
    }
    const semantic = semanticGeneratedTitle(chapter, language);
    if (semantic && index > lastBody) {
      chapter.title = semantic;
      backMatterStarted = true;
      continue;
    }
    if (index >= firstBody && index <= lastBody && !backMatterStarted) {
      chapter.title = language.toLowerCase().startsWith("de")
        ? `Kapitel ${chapterNumber}` : `Chapter ${chapterNumber}`;
      chapterNumber += 1;
      continue;
    }
    chapter.title = index > lastBody
      ? (language.toLowerCase().startsWith("de") ? "Anhang" : "Appendix")
      : (language.toLowerCase().startsWith("de") ? "Vorspann" : "Front matter");
  }
  return result.map((chapter) => ({
    ...chapter,
    showHeading: !/^(?:front matter|vorspann|appendix|anhang)$/iu.test(chapter.title)
  }));
}

export function parseEpub(bytes, { bookId, revision = 1, sourceName = "book.epub" } = {}) {
  if (!bookId) {
    throw new Error("bookId is required");
  }

  const { source, zip, opfPath, opfXml, manifest, coverId, guideCoverPaths } = readEpubPackage(bytes);
  const spineIds = parseSpine(opfXml);
  const title = firstXmlText(opfXml, "title") || path.basename(sourceName, path.extname(sourceName)) || "Untitled";
  const authors = allXmlText(opfXml, "creator");
  const language = firstXmlText(opfXml, "language") || "und";

  const contentItems = spineIds.length > 0
    ? spineIds.map((id) => manifest.get(id)).filter(Boolean)
    : Array.from(manifest.values()).filter(isContentItem);

  const chapters = [];
  const navigationEntries = extractNavigationEntries(zip, manifest, opfXml);
  for (const item of contentItems) {
    if (!isContentItem(item)) {
      continue;
    }
    const content = zip.readTextEntry(item.path);
    const itemNavigation = navigationEntries.filter((entry) => entry.path === item.path);
    const documentTitle = itemNavigation.find((entry) => !entry.fragment)?.title ?? fileTitle(item.path);
    chapters.push(...xhtmlToChapters(content, documentTitle, itemNavigation));
  }

  if (chapters.length === 0) {
    throw new Error("EPUB contains no readable spine content");
  }

  const normalizedChapters = normalizeEpubChapterTitles(chapters, language);
  const frontMatter = /^(?:front matter|vorspann|cover|title|titel|copyright|impressum|contents?|inhalt|dedication|widmung|motto|zitate?|quotes?|(?:über|ueber)\s+(?:dieses\s+buch|den\s+autor|die\s+autorin)|about\s+(?:this\s+book|the\s+author)|summary|synopsis|zusammenfassung|wohin\s+soll)\b/iu;
  const frontMatterText = /(?:all rights reserved|alle rechte vorbehalten|isbn\b|bibliografische information|copyright|©|zusammenfassung|in diesem buch|about this book)/iu;
  const bodyTitle = /^(?:prolog(?:ue)?|epilog(?:ue)?|\d+|(?:kapitel|chapter|tag|teil|part|book)\b|\d+\.?\s*(?:auftritt|stunden?|tage?|weeks?|months?))/iu;
  let recommendedIndex = normalizedChapters.findIndex((chapter) =>
    bodyTitle.test(chapter.title) && !frontMatter.test(chapter.title) && chapterWordCount(chapter) >= 20
  );
  if (recommendedIndex < 0) {
    recommendedIndex = normalizedChapters.findIndex((chapter) =>
      !frontMatter.test(chapter.title) &&
      !frontMatterText.test(chapter.paragraphs.slice(0, 5).join(" ")) &&
      chapterWordCount(chapter) >= 40
    );
  }
  if (recommendedIndex >= 0) normalizedChapters[recommendedIndex].recommendedStart = true;

  return {
    sourceHash: sha256(source),
    opfPath,
    metadata: {
      book_id: bookId,
      revision,
      title,
      authors,
      language,
      source_name: sourceName,
      source_hash: sha256(source)
    },
    cover: extractCover(zip, manifest, coverId, guideCoverPaths),
    chapters: normalizedChapters
  };
}

export function processEpubToReaderPackage(bytes, options) {
  const parsed = parseEpub(bytes, options);
  const readerPackage = buildReaderPackageFromChapters({
    bookId: parsed.metadata.book_id,
    revision: parsed.metadata.revision,
    title: parsed.metadata.title,
    authors: parsed.metadata.authors,
    language: parsed.metadata.language,
    chapters: parsed.chapters
  });

  return {
    ...parsed,
    readerPackage,
    readerPackageBytes: serializeReaderPackage(readerPackage)
  };
}
