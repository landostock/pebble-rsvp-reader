const HTML_ENTITIES = new Map([
  ["amp", "&"],
  ["apos", "'"],
  ["bdquo", "„"],
  ["bull", "•"],
  ["copy", "©"],
  ["gt", ">"],
  ["hellip", "…"],
  ["laquo", "«"],
  ["ldquo", "“"],
  ["lsaquo", "‹"],
  ["lsquo", "‘"],
  ["lt", "<"],
  ["mdash", "—"],
  ["ndash", "–"],
  ["nbsp", "\u00a0"],
  ["quot", "\""],
  ["raquo", "»"],
  ["rdquo", "”"],
  ["rsaquo", "›"],
  ["rsquo", "’"],
  ["thinsp", "\u2009"],
  ["Auml", "Ä"],
  ["Ouml", "Ö"],
  ["Uuml", "Ü"],
  ["auml", "ä"],
  ["ouml", "ö"],
  ["szlig", "ß"],
  ["uuml", "ü"]
]);

const SPACE_EQUIVALENTS = /[\t\f\v \u00a0\u1680\u180e\u2000-\u200a\u2028\u2029\u202f\u205f\u3000]+/gu;
const BLANK_LINE = /\n[ \t]*\n+/u;

export function decodeHtmlEntities(value) {
  return String(value).replace(/&(#x[0-9a-f]+|#[0-9]+|[a-z][a-z0-9]+);/giu, (match, entity) => {
    if (entity[0] === "#") {
      const radix = entity[1]?.toLowerCase() === "x" ? 16 : 10;
      const digits = radix === 16 ? entity.slice(2) : entity.slice(1);
      const codePoint = Number.parseInt(digits, radix);
      if (Number.isFinite(codePoint) && codePoint >= 0 && codePoint <= 0x10ffff) {
        return String.fromCodePoint(codePoint);
      }
      return " ";
    }
    return HTML_ENTITIES.get(entity) ?? match;
  });
}

export function normalizeLineHyphenation(value) {
  return String(value)
    .replace(/\u00ad/gu, "")
    .replace(/([\p{L}\p{N}])-\r?\n[ \t]*([\p{L}\p{N}])/gu, (_match, before, after) => {
      const beforeLower = before === before.toLocaleLowerCase() && before !== before.toLocaleUpperCase();
      const afterLower = after === after.toLocaleLowerCase() && after !== after.toLocaleUpperCase();
      return beforeLower && afterLower ? `${before}${after}` : `${before}-${after}`;
    });
}

export function normalizeReaderText(value) {
  return normalizeLineHyphenation(decodeHtmlEntities(value))
    .replace(/\r\n?/gu, "\n")
    .replace(SPACE_EQUIVALENTS, " ")
    .replace(/\n+/gu, " ")
    .replace(/ +/gu, " ")
    .trim()
    .normalize("NFC");
}

export function normalizeParagraphs(value) {
  const decoded = normalizeLineHyphenation(decodeHtmlEntities(value))
    .replace(/\r\n?/gu, "\n")
    .normalize("NFC");

  return decoded
    .split(BLANK_LINE)
    .map((paragraph) => paragraph.replace(SPACE_EQUIVALENTS, " ").replace(/\n+/gu, " ").replace(/ +/gu, " ").trim())
    .filter(Boolean);
}
