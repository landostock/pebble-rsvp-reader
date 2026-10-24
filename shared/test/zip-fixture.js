import { crc32 } from "../src/protocol.js";

function dosDateTime() {
  return { time: 0, date: 0 };
}

export function createZip(entries) {
  const localParts = [];
  const centralParts = [];
  let offset = 0;

  for (const entry of entries) {
    const name = Buffer.from(entry.name, "utf8");
    const data = Buffer.from(entry.data);
    const checksum = crc32(data);
    const { time, date } = dosDateTime();

    const localHeader = Buffer.alloc(30);
    localHeader.writeUInt32LE(0x04034b50, 0);
    localHeader.writeUInt16LE(20, 4);
    localHeader.writeUInt16LE(0, 6);
    localHeader.writeUInt16LE(0, 8);
    localHeader.writeUInt16LE(time, 10);
    localHeader.writeUInt16LE(date, 12);
    localHeader.writeUInt32LE(checksum, 14);
    localHeader.writeUInt32LE(data.length, 18);
    localHeader.writeUInt32LE(data.length, 22);
    localHeader.writeUInt16LE(name.length, 26);
    localHeader.writeUInt16LE(0, 28);

    localParts.push(localHeader, name, data);

    const centralHeader = Buffer.alloc(46);
    centralHeader.writeUInt32LE(0x02014b50, 0);
    centralHeader.writeUInt16LE(20, 4);
    centralHeader.writeUInt16LE(20, 6);
    centralHeader.writeUInt16LE(0, 8);
    centralHeader.writeUInt16LE(0, 10);
    centralHeader.writeUInt16LE(time, 12);
    centralHeader.writeUInt16LE(date, 14);
    centralHeader.writeUInt32LE(checksum, 16);
    centralHeader.writeUInt32LE(data.length, 20);
    centralHeader.writeUInt32LE(data.length, 24);
    centralHeader.writeUInt16LE(name.length, 28);
    centralHeader.writeUInt16LE(0, 30);
    centralHeader.writeUInt16LE(0, 32);
    centralHeader.writeUInt16LE(0, 34);
    centralHeader.writeUInt16LE(0, 36);
    centralHeader.writeUInt32LE(0, 38);
    centralHeader.writeUInt32LE(offset, 42);
    centralParts.push(centralHeader, name);

    offset += localHeader.length + name.length + data.length;
  }

  const centralDirectory = Buffer.concat(centralParts);
  const body = Buffer.concat(localParts);
  const eocd = Buffer.alloc(22);
  eocd.writeUInt32LE(0x06054b50, 0);
  eocd.writeUInt16LE(0, 4);
  eocd.writeUInt16LE(0, 6);
  eocd.writeUInt16LE(entries.length, 8);
  eocd.writeUInt16LE(entries.length, 10);
  eocd.writeUInt32LE(centralDirectory.length, 12);
  eocd.writeUInt32LE(body.length, 16);
  eocd.writeUInt16LE(0, 20);

  return Buffer.concat([body, centralDirectory, eocd]);
}

export function createEpubFixture({ encrypted = false, cover = false, coverBytes = Buffer.from("fake cover bytes", "utf8") } = {}) {
  const coverMetadata = cover === "metadata"
    ? '    <meta name="cover" content="cover-image"/>\n'
    : "";
  const coverProperties = cover === "properties" ? ' properties="cover-image"' : "";
  const coverManifest = cover === "guide-page"
    ? `    <item id="cover-page" href="cover.xhtml" media-type="application/xhtml+xml"/>
    <item id="front-image" href="images/frontispiece.jpg" media-type="image/jpeg"/>
`
    : cover
      ? `    <item id="cover-image" href="images/cover.jpg" media-type="image/jpeg"${coverProperties}/>\n`
      : "";
  const coverGuide = cover === "guide-page"
    ? `  <guide>
    <reference type="cover" title="Cover" href="cover.xhtml"/>
  </guide>
`
    : "";
  const entries = [
    {
      name: "mimetype",
      data: "application/epub+zip"
    },
    {
      name: "META-INF/container.xml",
      data: `<?xml version="1.0"?>
<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
  <rootfiles>
    <rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/>
  </rootfiles>
</container>`
    },
    {
      name: "OEBPS/content.opf",
      data: `<?xml version="1.0" encoding="utf-8"?>
<package version="3.0" xmlns:dc="http://purl.org/dc/elements/1.1/">
  <metadata>
    <dc:title>Spine Test</dc:title>
    <dc:creator>Fixture Author</dc:creator>
    <dc:language>de</dc:language>
${coverMetadata}
  </metadata>
  <manifest>
${coverManifest}
    <item id="chapter-one" href="chapter-one.xhtml" media-type="application/xhtml+xml"/>
    <item id="chapter-two" href="chapter-two.xhtml" media-type="application/xhtml+xml"/>
    <item id="style" href="style.css" media-type="text/css"/>
  </manifest>
  <spine>
    <itemref idref="chapter-two"/>
    <itemref idref="chapter-one"/>
  </spine>
${coverGuide}
</package>`
    },
    {
      name: "OEBPS/chapter-one.xhtml",
      data: `<?xml version="1.0" encoding="utf-8"?>
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="de">
  <head><title>Ignored title</title><style>p { color: red; }</style></head>
  <body>
    <h1>Erstes Kapitel</h1>
    <p>Da war&rsquo;s plötzlich still.</p>
    <figure><img src="images/scene.jpg" alt=""/><figcaption>Abbildung: Der stille Platz.</figcaption></figure>
    <p>E-Mail, Jean-Paul und Nord-Süd-Verbindung bleiben echt.</p>
  </body>
</html>`
    },
    {
      name: "OEBPS/chapter-two.xhtml",
      data: `<?xml version="1.0" encoding="utf-8"?>
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="de">
  <body>
    <nav><p>Table of contents must not become text.</p></nav>
    <h1>Zweites Kapitel</h1>
    <p>Er&nbsp;sagte: &bdquo;Hallo!&ldquo;</p>
    <p>Donaudampfschifffahrtsgesellschaft und Straßenverkehrsordnung.</p>
    <p>Schon komisch, d<span class="kursiv">ie Welt ist schön.</span></p>
  </body>
</html>`
    },
    {
      name: "OEBPS/style.css",
      data: "body { font-family: serif; }"
    }
  ];

  if (cover) {
    entries.push({
      name: cover === "guide-page" ? "OEBPS/images/frontispiece.jpg" : "OEBPS/images/cover.jpg",
      data: coverBytes
    });
  }

  if (cover === "guide-page") {
    entries.push({
      name: "OEBPS/cover.xhtml",
      data: `<?xml version="1.0" encoding="utf-8"?>
<html xmlns="http://www.w3.org/1999/xhtml">
  <body>
    <img src="images/frontispiece.jpg" alt="Cover"/>
  </body>
</html>`
    });
  }

  if (encrypted) {
    entries.push({
      name: "META-INF/encryption.xml",
      data: "<encryption/>"
    });
  }

  return createZip(entries);
}
