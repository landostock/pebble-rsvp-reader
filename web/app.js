const defaultSettings = Object.freeze({
  revision: 0,
  language: "en",
  library_sort: "last_read",
  library_view: "grid",
  display_theme: "system",
  library_density: "comfortable"
});

const externalRefreshIntervalMs = 5_000;
let externalRefreshInFlight = false;

const state = {
  csrf: "",
  authenticated: false,
  devNoAuth: false,
  authMode: "password",
  language: defaultSettings.language,
  settings: { ...defaultSettings },
  apiStatusKind: "",
  apiStatusKey: "apiChecking",
  library: null,
  diagnostics: null,
  deviceTokens: [],
  selectedBookId: null,
  view: "library",
  libraryMode: "grid"
};

const elements = {
  apiDot: document.querySelector("#api-dot"),
  apiStatus: document.querySelector("#api-status"),
  diagnosticsJson: document.querySelector("#diagnostics-json"),
  detailAuthors: document.querySelector("#detail-authors"),
  detailBackdrop: document.querySelector("#detail-backdrop"),
  detailClose: document.querySelector("#detail-close"),
  detailCover: document.querySelector("#detail-cover"),
  detailForm: document.querySelector("#detail-form"),
  detailHeading: document.querySelector("#detail-heading"),
  detailLanguage: document.querySelector("#detail-language"),
  detailMetadataRevision: document.querySelector("#detail-metadata-revision"),
  detailPane: document.querySelector("#detail-pane"),
  detailRevision: document.querySelector("#detail-revision"),
  detailSubheading: document.querySelector("#detail-subheading"),
  detailTitle: document.querySelector("#detail-title"),
  detailTokens: document.querySelector("#detail-tokens"),
  defaultSort: document.querySelector("#default-sort"),
  deleteBook: document.querySelector("#delete-book"),
  deviceName: document.querySelector("#device-name"),
  deviceTokenDialog: document.querySelector("#device-token-dialog"),
  deviceTokenError: document.querySelector("#device-token-error"),
  deviceTokenForm: document.querySelector("#device-token-form"),
  deviceTokenList: document.querySelector("#device-token-list"),
  deviceTokenOutput: document.querySelector("#device-token-output"),
  deviceTokenResult: document.querySelector("#device-token-result"),
  downloadPackage: document.querySelector("#download-package"),
  fileInput: document.querySelector("#file-input"),
  generateDeviceToken: document.querySelector("#generate-device-token"),
  coverInput: document.querySelector("#cover-input"),
  extractCover: document.querySelector("#extract-cover"),
  authButton: document.querySelector("#auth-button"),
  authCaption: document.querySelector("#auth-caption"),
  authSummary: document.querySelector("#auth-summary"),
  language: document.querySelector("#language"),
  library: document.querySelector("#library"),
  libraryEmpty: document.querySelector("#library-empty"),
  libraryEmptyTitle: document.querySelector("#library-empty-title"),
  libraryEmptyBody: document.querySelector("#library-empty-body"),
  libraryLayout: document.querySelector("#library-layout"),
  libraryToolbar: document.querySelector("#library-toolbar"),
  libraryView: document.querySelector("#library-view"),
  loginDialog: document.querySelector("#login-dialog"),
  loginError: document.querySelector("#login-error"),
  loginForm: document.querySelector("#login-form"),
  loginNote: document.querySelector("#login-note"),
  password: document.querySelector("#password"),
  detailProgressBar: document.querySelector("#detail-progress-bar"),
  progressSummary: document.querySelector("#progress-summary"),
  resetProgress: document.querySelector("#reset-progress"),
  search: document.querySelector("#search"),
  searchResults: document.querySelector("#search-results"),
  settingsStatus: document.querySelector("#settings-status"),
  sort: document.querySelector("#sort"),
  storedBytes: document.querySelector("#stored-bytes"),
  toast: document.querySelector("#toast"),
  topbarControls: document.querySelector("#topbar-controls"),
  uploadList: document.querySelector("#upload-list"),
  viewSubtitle: document.querySelector("#view-subtitle"),
  viewTitle: document.querySelector("#view-title")
};

const translations = {
  en: {
    brandSubtitle: "Pebble library",
    navLibrary: "Library",
    navUpload: "Upload",
    navSettings: "Settings",
    navDisplay: "Display",
    navDiagnostics: "Diagnostics",
    languageLabel: "Language",
    signIn: "Sign in",
    signOut: "Sign out",
    devMode: "Dev mode",
    devModeActive: "Dev mode active",
    signedIn: "Signed in",
    devSession: "Dev session",
    devAuthNotice: "Local development mode is active, so the server is not requiring a password.",
    protectedByPassword: "Password protected",
    signInToView: "Sign in to view this Pebble library.",
    signedInCaption: "Library unlocked",
    devModeCaption: "Local bypass only",
    signedOutCaption: "Password required",
    accessMode: "Access",
    accessDevSummary: "Development mode bypasses the password for local work. Do not use it for a shared instance.",
    accessPasswordLockedSummary: "This instance is protected. Sign in before viewing books, uploading EPUBs, changing covers, or downloading reader packages.",
    accessPasswordUnlockedSummary: "This instance is unlocked for your current browser session. Books and uploads remain behind the password for other visitors.",
    searchPlaceholder: "Search title or author",
    searchNoMatches: "No matching books",
    addBook: "Add book",
    sortLastRead: "Recently read",
    sortRecent: "Recently added",
    sortTitle: "Title A-Z",
    sortAuthor: "Author A-Z",
    sortProgress: "Progress",
    modeGrid: "Grid",
    modeList: "List",
    emptyTitle: "No books yet",
    emptyBody: "Upload an EPUB to create the first reader package.",
    uploadTitle: "EPUB Upload",
    uploadBody: "Ready, failed, and in-progress imports appear below.",
    chooseFiles: "Choose files",
    uploadRequiresSignIn: "Sign in before uploading EPUB files.",
    diagnosticsRequiresSignIn: "Sign in before viewing diagnostics.",
    selectedBook: "Selected Book",
    canonicalPosition: "Canonical Position",
    androidCompanion: "Android Companion",
    deviceTokenSummary: "Give each phone its own revocable access. The administrator password never leaves the browser.",
    generateToken: "Generate device token",
    pairDevice: "Pair Android phone",
    pairDeviceSummary: "Name this phone so its access can be revoked later.",
    deviceName: "Device name",
    deviceNamePlaceholder: "Pixel 10",
    noPairedDevices: "No paired phones yet.",
    revokeToken: "Revoke",
    revokeTokenConfirm: "Revoke access for this phone?",
    tokenCreated: "Device access created",
    tokenRevoked: "Device access revoked",
    pairingReady: "Pairing data ready",
    tokenShownOnce: "This secret is shown once. Copy it into the Android companion now.",
    copyPairingData: "Copy pairing data",
    pairingDataCopied: "Pairing data copied",
    cancel: "Cancel",
    done: "Done",
    storageGuard: "Storage Guard",
    generalSettings: "General",
    languageSummary: "Used by the web app and synchronized watch interface.",
    defaultSort: "Default library order",
    defaultSortSummary: "The phone and watch receive the same order.",
    appearanceSettings: "Appearance",
    themeLabel: "Theme",
    themeSummary: "Follow the system or choose a fixed appearance.",
    themeSystem: "System",
    themeLight: "Light",
    themeDark: "Dark",
    libraryViewLabel: "Library view",
    libraryViewSummary: "Choose the default layout for browsing books.",
    densityLabel: "Density",
    densitySummary: "Compact shows more books without reducing readability.",
    densityComfortable: "Comfortable",
    densityCompact: "Compact",
    previewTitle: "A well-kept library",
    previewAuthor: "Books remain the visual focus.",
    settingsSaving: "Saving...",
    settingsSaved: "Saved and queued for device sync.",
    settingsSaveFailed: "Could not save settings.",
    storedBytes: "Approximate stored bytes",
    sourceLimit: "Default source limit",
    warningThreshold: "Warning threshold",
    backendDiagnostics: "Backend Diagnostics",
    bookDetails: "Book details",
    closeDetails: "Close book details",
    noBookSelected: "No book selected.",
    uploadCover: "Upload cover",
    extractCover: "Use EPUB cover",
    extractingCover: "Reading EPUB cover",
    extractedCover: "EPUB cover applied",
    noEmbeddedCover: "No embedded EPUB cover found",
    coverUploading: "Uploading cover",
    coverUploaded: "Cover updated",
    coverUploadFailed: "Cover upload failed",
    deleteBook: "Delete book",
    deleteConfirm: "Delete this book from the library?",
    deletedBook: "Book deleted",
    deleteFailed: "Delete failed",
    fieldTitle: "Title",
    fieldAuthors: "Authors",
    fieldLanguage: "Language",
    tokens: "Tokens",
    revision: "Revision",
    metadataRevision: "Metadata",
    saveMetadata: "Save metadata",
    downloadPackage: "Download reader package",
    readingProgress: "Reading progress",
    resetProgress: "Mark as unread",
    resetProgressConfirm: "Mark this book as unread? Its reading position and progress will be reset. Bookmarks will be kept.",
    progressReset: "Book marked as unread",
    progressResetFailed: "Could not reset reading progress",
    passwordLabel: "Administrator password",
    viewLibraryTitle: "Library",
    viewLibrarySubtitle: "Watch-ready reader packages.",
    viewUploadTitle: "Upload",
    viewUploadSubtitle: "EPUB imports and processing state.",
    viewSettingsTitle: "Settings",
    viewSettingsSubtitle: "Language, library order, access, and companion setup.",
    viewDisplayTitle: "Display",
    viewDisplaySubtitle: "Appearance and library layout.",
    viewDiagnosticsTitle: "Diagnostics",
    viewDiagnosticsSubtitle: "Backend and protocol state.",
    apiChecking: "Checking API",
    apiReady: "API ready",
    apiUnavailable: "API unavailable",
    signInRequired: "Sign in required",
    selectBook: "Select a book from Library.",
    noProgress: "No progress stored.",
    unknownAuthor: "Unknown author",
    tokensLower: "tokens",
    uploading: "Uploading",
    dropToUpload: "Drop EPUB files to add them",
    ready: "Ready",
    failed: "Failed",
    metadataSaved: "Metadata saved",
    isReady: "is ready",
    authenticationRequired: "Authentication required",
    invalidPassword: "Invalid password",
    signedOut: "Signed out"
  },
  de: {
    brandSubtitle: "Pebble-Bibliothek",
    navLibrary: "Bibliothek",
    navUpload: "Upload",
    navSettings: "Einstellungen",
    navDisplay: "Anzeige",
    navDiagnostics: "Diagnose",
    languageLabel: "Sprache",
    signIn: "Anmelden",
    signOut: "Abmelden",
    devMode: "Dev-Modus",
    devModeActive: "Dev-Modus aktiv",
    signedIn: "Angemeldet",
    devSession: "Dev-Sitzung",
    devAuthNotice: "Lokaler Entwicklungsmodus ist aktiv, deshalb verlangt der Server kein Passwort.",
    protectedByPassword: "Passwortgeschützt",
    signInToView: "Melde dich an, um diese Pebble-Bibliothek zu sehen.",
    signedInCaption: "Bibliothek entsperrt",
    devModeCaption: "Nur lokaler Bypass",
    signedOutCaption: "Passwort erforderlich",
    accessMode: "Zugriff",
    accessDevSummary: "Der Entwicklungsmodus umgeht das Passwort nur für lokale Arbeit. Für eine geteilte Instanz darf er nicht aktiv sein.",
    accessPasswordLockedSummary: "Diese Instanz ist geschützt. Melde dich an, bevor du Bücher siehst, EPUBs hochlädst, Cover änderst oder Reader-Pakete lädst.",
    accessPasswordUnlockedSummary: "Diese Instanz ist für deine aktuelle Browser-Sitzung entsperrt. Für andere Besucher bleiben Bücher und Uploads passwortgeschützt.",
    searchPlaceholder: "Titel oder Autor suchen",
    searchNoMatches: "Keine passenden Bücher",
    addBook: "Buch hinzufügen",
    sortLastRead: "Zuletzt gelesen",
    sortRecent: "Zuletzt hinzugefügt",
    sortTitle: "Titel A-Z",
    sortAuthor: "Autor A-Z",
    sortProgress: "Fortschritt",
    modeGrid: "Raster",
    modeList: "Liste",
    emptyTitle: "Noch keine Bücher",
    emptyBody: "Lade ein EPUB hoch, um das erste Reader-Paket zu erstellen.",
    uploadTitle: "EPUB-Upload",
    uploadBody: "Fertige, fehlgeschlagene und laufende Importe erscheinen unten.",
    chooseFiles: "Dateien wählen",
    uploadRequiresSignIn: "Melde dich an, bevor du EPUB-Dateien hochlädst.",
    diagnosticsRequiresSignIn: "Melde dich an, um die Diagnose zu sehen.",
    selectedBook: "Ausgewähltes Buch",
    canonicalPosition: "Kanonische Position",
    androidCompanion: "Android-Begleitapp",
    deviceTokenSummary: "Gib jedem Telefon einen eigenen widerrufbaren Zugriff. Das Administratorpasswort bleibt im Browser.",
    generateToken: "Geräte-Token erzeugen",
    pairDevice: "Android-Telefon koppeln",
    pairDeviceSummary: "Benenne dieses Telefon, damit sein Zugriff später gezielt widerrufen werden kann.",
    deviceName: "Gerätename",
    deviceNamePlaceholder: "Pixel 10",
    noPairedDevices: "Noch keine Telefone gekoppelt.",
    revokeToken: "Widerrufen",
    revokeTokenConfirm: "Den Zugriff für dieses Telefon widerrufen?",
    tokenCreated: "Gerätezugriff erstellt",
    tokenRevoked: "Gerätezugriff widerrufen",
    pairingReady: "Kopplungsdaten bereit",
    tokenShownOnce: "Dieses Geheimnis wird nur einmal angezeigt. Kopiere es jetzt in die Android-Begleitapp.",
    copyPairingData: "Kopplungsdaten kopieren",
    pairingDataCopied: "Kopplungsdaten kopiert",
    cancel: "Abbrechen",
    done: "Fertig",
    storageGuard: "Speichergrenze",
    generalSettings: "Allgemein",
    languageSummary: "Gilt für die Web-App und wird mit der Uhr synchronisiert.",
    defaultSort: "Standardreihenfolge der Bibliothek",
    defaultSortSummary: "Telefon und Uhr erhalten dieselbe Reihenfolge.",
    appearanceSettings: "Darstellung",
    themeLabel: "Farbschema",
    themeSummary: "Dem System folgen oder eine feste Darstellung wählen.",
    themeSystem: "System",
    themeLight: "Hell",
    themeDark: "Dunkel",
    libraryViewLabel: "Bibliotheksansicht",
    libraryViewSummary: "Lege die Standarddarstellung für Bücher fest.",
    densityLabel: "Dichte",
    densitySummary: "Kompakt zeigt mehr Bücher, ohne die Lesbarkeit zu verringern.",
    densityComfortable: "Komfortabel",
    densityCompact: "Kompakt",
    previewTitle: "Eine aufgeräumte Bibliothek",
    previewAuthor: "Die Bücher bleiben im Mittelpunkt.",
    settingsSaving: "Wird gespeichert...",
    settingsSaved: "Gespeichert und für die Gerätesynchronisierung vorgemerkt.",
    settingsSaveFailed: "Einstellungen konnten nicht gespeichert werden.",
    storedBytes: "Ungefähre Speichernutzung",
    sourceLimit: "Standardlimit für Quellen",
    warningThreshold: "Warnschwelle",
    backendDiagnostics: "Backend-Diagnose",
    bookDetails: "Buchdetails",
    closeDetails: "Buchdetails schließen",
    noBookSelected: "Kein Buch ausgewählt.",
    uploadCover: "Cover hochladen",
    extractCover: "EPUB-Cover verwenden",
    extractingCover: "EPUB-Cover wird gelesen",
    extractedCover: "EPUB-Cover übernommen",
    noEmbeddedCover: "Kein eingebettetes EPUB-Cover gefunden",
    coverUploading: "Cover wird hochgeladen",
    coverUploaded: "Cover aktualisiert",
    coverUploadFailed: "Cover-Upload fehlgeschlagen",
    deleteBook: "Buch löschen",
    deleteConfirm: "Dieses Buch aus der Bibliothek löschen?",
    deletedBook: "Buch gelöscht",
    deleteFailed: "Löschen fehlgeschlagen",
    fieldTitle: "Titel",
    fieldAuthors: "Autoren",
    fieldLanguage: "Sprache",
    tokens: "Tokens",
    revision: "Revision",
    metadataRevision: "Metadaten",
    saveMetadata: "Metadaten speichern",
    downloadPackage: "Reader-Paket laden",
    readingProgress: "Lesefortschritt",
    resetProgress: "Als ungelesen markieren",
    resetProgressConfirm: "Dieses Buch als ungelesen markieren? Leseposition und Fortschritt werden zurückgesetzt. Lesezeichen bleiben erhalten.",
    progressReset: "Buch als ungelesen markiert",
    progressResetFailed: "Lesefortschritt konnte nicht zurückgesetzt werden",
    passwordLabel: "Administrator-Passwort",
    viewLibraryTitle: "Bibliothek",
    viewLibrarySubtitle: "Watch-fertige Reader-Pakete.",
    viewUploadTitle: "Upload",
    viewUploadSubtitle: "EPUB-Importe und Verarbeitungsstatus.",
    viewSettingsTitle: "Einstellungen",
    viewSettingsSubtitle: "Sprache, Bibliotheksreihenfolge, Zugriff und Begleitapp.",
    viewDisplayTitle: "Anzeige",
    viewDisplaySubtitle: "Darstellung und Bibliothekslayout.",
    viewDiagnosticsTitle: "Diagnose",
    viewDiagnosticsSubtitle: "Backend- und Protokollstatus.",
    apiChecking: "API wird geprüft",
    apiReady: "API bereit",
    apiUnavailable: "API nicht erreichbar",
    signInRequired: "Anmeldung erforderlich",
    selectBook: "Wähle ein Buch in der Bibliothek aus.",
    noProgress: "Kein Fortschritt gespeichert.",
    unknownAuthor: "Unbekannter Autor",
    tokensLower: "Tokens",
    uploading: "Upload läuft",
    dropToUpload: "EPUB-Dateien ablegen zum Hinzufügen",
    ready: "Bereit",
    failed: "Fehlgeschlagen",
    metadataSaved: "Metadaten gespeichert",
    isReady: "ist bereit",
    authenticationRequired: "Anmeldung erforderlich",
    invalidPassword: "Ungültiges Passwort",
    signedOut: "Abgemeldet"
  }
};

const viewKeys = {
  library: ["viewLibraryTitle", "viewLibrarySubtitle"],
  settings: ["viewSettingsTitle", "viewSettingsSubtitle"],
  diagnostics: ["viewDiagnosticsTitle", "viewDiagnosticsSubtitle"]
};

if (!translations[state.language]) {
  state.language = "en";
}

function t(key) {
  return translations[state.language]?.[key] ?? translations.en[key] ?? key;
}

function settingChoice(value, choices, fallback) {
  return choices.includes(value) ? value : fallback;
}

function normalizeSettings(settings = {}) {
  return {
    ...defaultSettings,
    ...settings,
    revision: Math.max(0, Math.trunc(Number(settings.revision) || 0)),
    language: settingChoice(settings.language, ["en", "de"], defaultSettings.language),
    library_sort: settingChoice(settings.library_sort, ["last_read", "title", "author", "recent", "progress"], defaultSettings.library_sort),
    library_view: settingChoice(settings.library_view, ["grid", "list"], defaultSettings.library_view),
    display_theme: settingChoice(settings.display_theme, ["system", "light", "dark"], defaultSettings.display_theme),
    library_density: settingChoice(settings.library_density, ["comfortable", "compact"], defaultSettings.library_density)
  };
}

function applyTheme() {
  const requested = state.settings.display_theme;
  const resolved = requested === "system"
    ? (window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light")
    : requested;
  document.documentElement.dataset.theme = resolved;
  document.documentElement.dataset.density = state.settings.library_density;
}

function syncSettingsControls() {
  elements.language.value = state.settings.language;
  elements.defaultSort.value = state.settings.library_sort;
  elements.sort.value = state.settings.library_sort;
  document.querySelectorAll("[data-library-mode]").forEach((button) => {
    button.classList.toggle("active", button.dataset.libraryMode === state.libraryMode);
  });
}

function applySettings(settings) {
  const previousLanguage = state.language;
  state.settings = normalizeSettings(settings);
  state.language = state.settings.language;
  state.libraryMode = state.settings.library_view;
  syncSettingsControls();
  applyTheme();
  return previousLanguage !== state.language;
}

function applyTranslations() {
  document.documentElement.lang = state.language;
  syncSettingsControls();
  document.querySelectorAll("[data-i18n]").forEach((element) => {
    element.textContent = t(element.dataset.i18n);
  });
  document.querySelectorAll("[data-i18n-placeholder]").forEach((element) => {
    element.setAttribute("placeholder", t(element.dataset.i18nPlaceholder));
  });
  document.querySelector(".add-book-action")?.setAttribute("aria-label", t("addBook"));
  document.querySelector(".add-book-action")?.setAttribute("title", t("addBook"));
  elements.detailBackdrop.setAttribute("aria-label", t("closeDetails"));
  elements.detailClose.setAttribute("aria-label", t("closeDetails"));
  elements.detailClose.setAttribute("title", t("closeDetails"));
  updateAuthButton();
  updateLoginMode();
  updateAuthSummary();
  updateProtectedControls();
  setApiStatus(state.apiStatusKind, state.apiStatusKey);
  updateViewHeading();
  renderLibrary();
  renderSelectedBook();
  renderSearchResults();
  renderDeviceTokens();
}

function updateViewHeading() {
  const [titleKey, subtitleKey] = viewKeys[state.view] ?? viewKeys.library;
  elements.viewTitle.textContent = t(titleKey);
  elements.viewSubtitle.textContent = t(subtitleKey);
  elements.topbarControls.classList.toggle("hidden", state.view !== "library");
}

function updateAuthButton() {
  if (state.devNoAuth) {
    elements.authButton.textContent = t("devModeActive");
    elements.authButton.title = t("devAuthNotice");
    elements.authButton.disabled = true;
    elements.authButton.classList.add("mode-only");
    elements.authCaption.textContent = t("devModeCaption");
    return;
  }
  elements.authButton.disabled = false;
  elements.authButton.classList.remove("mode-only");
  elements.authButton.textContent = state.authenticated ? t("signOut") : t("signIn");
  elements.authButton.title = "";
  elements.authCaption.textContent = state.authenticated ? t("signedInCaption") : t("signedOutCaption");
}

function updateLoginMode() {
  elements.loginNote.classList.toggle("hidden", !state.devNoAuth);
  elements.password.closest("label").classList.toggle("hidden", state.devNoAuth);
}

function updateAuthSummary() {
  if (state.devNoAuth) {
    elements.authSummary.textContent = t("accessDevSummary");
  } else if (state.authenticated) {
    elements.authSummary.textContent = t("accessPasswordUnlockedSummary");
  } else {
    elements.authSummary.textContent = t("accessPasswordLockedSummary");
  }
}

function updateProtectedControls() {
  const locked = !state.authenticated;
  elements.fileInput.disabled = locked;
  elements.coverInput.disabled = locked;
  elements.extractCover.disabled = locked;
  elements.deleteBook.disabled = locked;
  elements.resetProgress.disabled = locked;
  elements.generateDeviceToken.disabled = locked;
  elements.search.disabled = locked;
  elements.sort.disabled = locked;
  elements.language.disabled = locked;
  elements.defaultSort.disabled = locked;
  document.querySelectorAll("[data-library-mode]").forEach((button) => {
    button.disabled = locked;
  });
  elements.libraryView.classList.toggle("locked", locked);
  elements.libraryToolbar.classList.toggle("locked", locked);
  if (locked) {
    elements.diagnosticsJson.textContent = t("diagnosticsRequiresSignIn");
    elements.storedBytes.textContent = "-";
  }
}

function showToast(message) {
  elements.toast.textContent = message;
  elements.toast.classList.remove("hidden");
  window.clearTimeout(showToast.timeout);
  showToast.timeout = window.setTimeout(() => elements.toast.classList.add("hidden"), 3200);
}

function setApiStatus(kind, labelKey) {
  state.apiStatusKind = kind;
  state.apiStatusKey = labelKey;
  elements.apiDot.className = `status-dot ${kind}`;
  elements.apiStatus.textContent = t(labelKey);
}

function formatBytes(bytes) {
  if (!Number.isFinite(bytes)) {
    return "-";
  }
  const units = ["B", "KB", "MB", "GB"];
  let value = bytes;
  let unit = 0;
  while (value >= 1024 && unit + 1 < units.length) {
    value /= 1024;
    unit += 1;
  }
  return `${value.toFixed(value >= 10 || unit === 0 ? 0 : 1)} ${units[unit]}`;
}

function imageContentType(file) {
  if (file.type.startsWith("image/")) {
    return file.type;
  }
  const extension = file.name.toLowerCase().split(".").pop();
  return {
    gif: "image/gif",
    jpeg: "image/jpeg",
    jpg: "image/jpeg",
    png: "image/png",
    webp: "image/webp"
  }[extension] ?? "";
}

async function api(path, options = {}) {
  const headers = new Headers(options.headers ?? {});
  if (options.body && !(options.body instanceof File) && !(options.body instanceof Blob) && !headers.has("content-type")) {
    headers.set("content-type", "application/json");
  }
  if (options.method && options.method !== "GET" && state.csrf) {
    headers.set("x-csrf-token", state.csrf);
  }

  const response = await fetch(path, {
    credentials: "same-origin",
    ...options,
    headers
  });
  if (response.status === 401 && path === "/api/session" && options.method === "POST") {
    let message = response.statusText;
    try {
      const payload = await response.json();
      message = payload.error?.code === "invalid_credentials" ? t("invalidPassword") : payload.error?.message ?? message;
    } catch {
      // Keep the HTTP status text.
    }
    throw new Error(message);
  }
  if (response.status === 401) {
    if (!elements.loginDialog.open) {
      elements.loginDialog.showModal();
    }
    throw new Error(t("authenticationRequired"));
  }
  if (!response.ok) {
    let message = response.statusText;
    try {
      message = (await response.json()).error?.message ?? message;
    } catch {
      // Keep the HTTP status text.
    }
    throw new Error(message);
  }
  const contentType = response.headers.get("content-type") ?? "";
  return contentType.includes("application/json") ? response.json() : response.arrayBuffer();
}

async function refreshSession() {
  const session = await api("/api/session");
  state.devNoAuth = Boolean(session.dev_no_auth);
  state.authMode = session.auth_mode ?? (state.devNoAuth ? "development" : "password");
  state.authenticated = Boolean(session.authenticated);
  state.csrf = session.csrf ?? "";
  updateAuthButton();
  updateLoginMode();
  updateAuthSummary();
  updateProtectedControls();
}

async function refreshDiagnostics() {
  state.diagnostics = await api("/api/diagnostics");
  elements.diagnosticsJson.textContent = JSON.stringify(state.diagnostics, null, 2);
  elements.storedBytes.textContent = formatBytes(state.diagnostics.tigris_storage_usage);
}

async function refreshDeviceTokens() {
  const collection = await api("/api/device-tokens");
  state.deviceTokens = collection.tokens ?? [];
  renderDeviceTokens();
}

async function saveSettings(patch, statusElement) {
  statusElement.textContent = t("settingsSaving");
  try {
    const updated = await api("/api/settings", {
      method: "PUT",
      body: JSON.stringify(patch)
    });
    if (state.library) {
      state.library.settings = updated;
    }
    applySettings(updated);
    applyTranslations();
    statusElement.textContent = t("settingsSaved");
  } catch (error) {
    syncSettingsControls();
    statusElement.textContent = `${t("settingsSaveFailed")} ${error.message}`;
  }
}

function renderDeviceTokens() {
  elements.deviceTokenList.innerHTML = "";
  if (!state.authenticated || state.deviceTokens.length === 0) {
    const empty = document.createElement("p");
    empty.className = "device-token-empty";
    empty.textContent = t("noPairedDevices");
    elements.deviceTokenList.append(empty);
    return;
  }

  for (const token of state.deviceTokens) {
    const row = document.createElement("div");
    row.className = "device-token-row";
    row.innerHTML = "<div><strong></strong><span></span></div><button type=\"button\" class=\"danger-action\"></button>";
    row.querySelector("strong").textContent = token.name;
    row.querySelector("span").textContent = new Intl.DateTimeFormat(state.language, {
      dateStyle: "medium"
    }).format(new Date(token.created_at));
    const revoke = row.querySelector("button");
    revoke.textContent = t("revokeToken");
    revoke.disabled = !state.authenticated;
    revoke.addEventListener("click", async () => {
      if (!window.confirm(`${t("revokeTokenConfirm")}\n\n${token.name}`)) {
        return;
      }
      try {
        await api(`/api/device-tokens/${token.token_id}`, { method: "DELETE" });
        await refreshDeviceTokens();
        showToast(t("tokenRevoked"));
      } catch (error) {
        showToast(error.message);
      }
    });
    elements.deviceTokenList.append(row);
  }
}

async function refreshLibrary({ preserveDetail = false } = {}) {
  state.library = await api("/api/library");
  const languageChanged = applySettings(state.library.settings);
  if (state.selectedBookId && !state.library.books.some((book) => book.book_id === state.selectedBookId)) {
    state.selectedBookId = null;
  }
  if (languageChanged) {
    applyTranslations();
    return;
  }
  renderLibrary();
  if (preserveDetail && selectedBook()) {
    loadProgress(state.selectedBookId);
  } else {
    renderSelectedBook();
  }
}

async function refreshAll() {
  setApiStatus("", "apiChecking");
  await refreshDiagnostics();
  await refreshLibrary();
  setApiStatus("ok", "apiReady");
}

function externalRefreshAllowed() {
  if (!state.authenticated || document.visibilityState !== "visible" || !document.hasFocus()) {
    return false;
  }
  if (document.querySelector("dialog[open]")) {
    return false;
  }
  const active = document.activeElement;
  return !active || !active.matches("input, select, textarea, [contenteditable='true']");
}

async function refreshExternalState() {
  if (externalRefreshInFlight || !externalRefreshAllowed()) {
    return;
  }
  externalRefreshInFlight = true;
  try {
    await refreshLibrary({ preserveDetail: true });
    setApiStatus("ok", "apiReady");
  } catch (error) {
    if (error.message === t("authenticationRequired")) {
      state.authenticated = false;
      updateAuthButton();
      updateLoginMode();
      updateAuthSummary();
      updateProtectedControls();
    }
    setApiStatus("bad", "apiUnavailable");
  } finally {
    externalRefreshInFlight = false;
  }
}

function matchingBooks() {
  const query = elements.search.value.trim().toLocaleLowerCase();
  return [...(state.library?.books ?? [])].filter((book) => {
    const haystack = `${book.title} ${(book.authors ?? []).join(" ")}`.toLocaleLowerCase();
    return !query || haystack.includes(query);
  });
}

function progressPercentage(tokenIndex, tokenCount) {
  const index = Number(tokenIndex ?? 0);
  const total = Number(tokenCount ?? 0);
  if (!Number.isFinite(index) || !Number.isFinite(total) || total <= 1) return 0;
  return Math.max(0, Math.min(100, Math.round((index / (total - 1)) * 1000) / 10));
}

function sortedBooks() {
  const books = matchingBooks();
  const collator = new Intl.Collator(state.language, { numeric: true, sensitivity: "base" });
  const byTitle = (a, b) => collator.compare(a.title ?? "", b.title ?? "");
  switch (elements.sort.value) {
    case "title":
      books.sort(byTitle);
      break;
    case "author":
      books.sort((a, b) => collator.compare(a.authors?.[0] ?? "", b.authors?.[0] ?? "") || byTitle(a, b));
      break;
    case "progress":
      books.sort((a, b) =>
        progressPercentage(b.progress_token_index, b.token_count) -
        progressPercentage(a.progress_token_index, a.token_count) || byTitle(a, b));
      break;
    case "recent":
      books.sort((a, b) => String(b.created_at ?? "").localeCompare(String(a.created_at ?? "")) || byTitle(a, b));
      break;
    default:
      books.sort((a, b) => {
        const aHasProgress = Number(a.progress_token_index ?? 0) > 0 || Number(a.progress_furthest_token_index ?? 0) > 0;
        const bHasProgress = Number(b.progress_token_index ?? 0) > 0 || Number(b.progress_furthest_token_index ?? 0) > 0;
        if (aHasProgress !== bHasProgress) return aHasProgress ? -1 : 1;
        const aReadAt = Date.parse(a.progress_updated_at ?? "") || 0;
        const bReadAt = Date.parse(b.progress_updated_at ?? "") || 0;
        return bReadAt - aReadAt || String(b.created_at ?? "").localeCompare(String(a.created_at ?? "")) || byTitle(a, b);
      });
  }
  return books;
}

function renderSearchResults() {
  const query = elements.search.value.trim();
  if (!query || state.view === "library") {
    elements.searchResults.classList.add("hidden");
    elements.searchResults.innerHTML = "";
    return;
  }

  const matches = matchingBooks().slice(0, 6);
  elements.searchResults.innerHTML = "";
  elements.searchResults.classList.remove("hidden");
  if (matches.length === 0) {
    const empty = document.createElement("div");
    empty.className = "search-result empty";
    empty.textContent = t("searchNoMatches");
    elements.searchResults.append(empty);
    return;
  }

  for (const book of matches) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "search-result";
    button.innerHTML = "<strong></strong><span></span>";
    button.querySelector("strong").textContent = book.title || "Untitled";
    button.querySelector("span").textContent = (book.authors ?? []).join(", ") || t("unknownAuthor");
    button.addEventListener("click", () => {
      selectBook(book.book_id);
      switchView("library");
      elements.searchResults.classList.add("hidden");
    });
    elements.searchResults.append(button);
  }
}

function coverText(title) {
  return String(title ?? "Untitled")
    .split(/\s+/u)
    .slice(0, 4)
    .join(" ");
}

function coverUrl(book) {
  return book.cover_revision > 0 ? `/api/books/${book.book_id}/cover?v=${book.cover_revision}` : "";
}

function renderTextCover(container, title) {
  container.classList.remove("has-image");
  container.innerHTML = "<span></span>";
  container.querySelector("span").textContent = coverText(title);
}

function renderCover(container, book) {
  const url = coverUrl(book);
  if (!url) {
    renderTextCover(container, book.title);
    return;
  }

  container.innerHTML = "";
  container.classList.add("has-image");
  const image = document.createElement("img");
  image.src = url;
  image.alt = book.title ? `${book.title} cover` : "Book cover";
  image.addEventListener("error", () => renderTextCover(container, book.title), { once: true });
  container.append(image);
}

function renderLibrary() {
  const books = sortedBooks();
  elements.library.classList.toggle("list", state.libraryMode === "list");
  elements.library.innerHTML = "";
  if (!state.authenticated) {
    elements.libraryEmptyTitle.textContent = t("protectedByPassword");
    elements.libraryEmptyBody.textContent = t("signInToView");
    elements.libraryEmpty.classList.remove("hidden");
    return;
  }

  elements.libraryEmptyTitle.textContent = t("emptyTitle");
  elements.libraryEmptyBody.textContent = t("emptyBody");
  elements.libraryEmpty.classList.toggle("hidden", books.length > 0);

  for (const book of books) {
    const card = document.createElement("button");
    card.type = "button";
    card.className = `book-card ${book.book_id === state.selectedBookId ? "selected" : ""}`;
    card.dataset.bookId = book.book_id;
    card.title = book.title || "Untitled";
    const progress = progressPercentage(book.progress_token_index, book.token_count);
    card.innerHTML = `
      <div class="cover"><span></span></div>
      <div>
        <div class="book-title"></div>
        <div class="book-author"></div>
      </div>
      <div class="book-progress-row">
        <div class="book-progress" aria-label="Progress"><span style="width: ${progress}%"></span></div>
        <span class="book-progress-value">${progress.toFixed(1)}%</span>
      </div>
    `;
    renderCover(card.querySelector(".cover"), book);
    card.querySelector(".book-title").textContent = book.title || "Untitled";
    card.querySelector(".book-author").textContent = (book.authors ?? []).join(", ") || t("unknownAuthor");
    card.addEventListener("click", () => {
      selectBook(book.book_id);
    });
    elements.library.append(card);
  }
}

function selectBook(bookId) {
  state.selectedBookId = bookId;
  renderLibrary();
  renderSelectedBook();
  elements.detailClose.focus();
}

function closeDetails({ restoreFocus = true } = {}) {
  const previousBookId = state.selectedBookId;
  state.selectedBookId = null;
  renderLibrary();
  renderSelectedBook();
  if (restoreFocus && previousBookId) {
    elements.library.querySelector(`[data-book-id="${CSS.escape(previousBookId)}"]`)?.focus();
  }
}

function selectedBook() {
  return (state.library?.books ?? []).find((book) => book.book_id === state.selectedBookId) ?? null;
}

function renderSelectedBook() {
  const book = selectedBook();
  document.body.classList.toggle("detail-open", Boolean(book));
  elements.detailBackdrop.classList.toggle("hidden", !book);
  elements.detailPane.classList.toggle("hidden", !book);
  if (!book) {
    elements.progressSummary.textContent = t("selectBook");
    elements.detailProgressBar.style.width = "0%";
    return;
  }

  renderCover(elements.detailCover, book);
  elements.detailHeading.textContent = book.title || "Untitled";
  elements.detailSubheading.textContent = (book.authors ?? []).join(", ") || t("unknownAuthor");
  elements.detailTitle.value = book.title ?? "";
  elements.detailAuthors.value = (book.authors ?? []).join(", ");
  elements.detailLanguage.value = book.language ?? "und";
  elements.detailTokens.textContent = String(book.token_count ?? 0);
  elements.detailRevision.textContent = String(book.revision ?? "-");
  elements.detailMetadataRevision.textContent = String(book.metadata_revision ?? "-");
  loadProgress(book.book_id);
}

function progressSummaryText(progress, book) {
  if (!progress || progress.reset) {
    return t("noProgress");
  }
  const tokenIndex = Number(progress.token_index ?? 0);
  const tokenCount = Number(book?.token_count ?? 0);
  if (!Number.isFinite(tokenIndex) || !Number.isFinite(tokenCount) || tokenCount <= 0) {
    return t("noProgress");
  }
  const percentage = progressPercentage(tokenIndex, tokenCount).toFixed(1);
  return state.language === "de"
    ? `${percentage}% · Token ${tokenIndex} von ${tokenCount}`
    : `${percentage}% · token ${tokenIndex} of ${tokenCount}`;
}

async function loadProgress(bookId) {
  try {
    const book = selectedBook();
    const progress = await api(`/api/progress/${bookId}`);
    elements.progressSummary.textContent = progressSummaryText(progress, book);
    elements.detailProgressBar.style.width = `${progressPercentage(progress?.token_index, book?.token_count)}%`;
  } catch (error) {
    elements.progressSummary.textContent = error.message;
    elements.detailProgressBar.style.width = "0%";
  }
}

function switchView(view) {
  if (view !== "library" && state.selectedBookId) {
    closeDetails({ restoreFocus: false });
  }
  state.view = view;
  document.querySelectorAll(".nav-item").forEach((button) => button.classList.toggle("active", button.dataset.view === view));
  document.querySelectorAll(".view").forEach((section) => section.classList.toggle("active", section.id === `${view}-view`));
  updateViewHeading();
  renderSearchResults();
  if (!state.authenticated) {
    if (view === "diagnostics") {
      elements.diagnosticsJson.textContent = t("diagnosticsRequiresSignIn");
    }
    return;
  }
  if (view === "diagnostics" || view === "settings") {
    refreshDiagnostics().catch((error) => showToast(error.message));
  }
  if (view === "settings") {
    refreshDeviceTokens().catch((error) => showToast(error.message));
  }
}

async function uploadFiles(files) {
  const epubFiles = Array.from(files ?? []).filter((file) => file.name.toLowerCase().endsWith(".epub"));
  if (epubFiles.length === 0) {
    return;
  }
  if (!state.authenticated) {
    showToast(t("uploadRequiresSignIn"));
    return;
  }
  elements.uploadList.classList.remove("hidden");
  for (const file of epubFiles) {
    const row = document.createElement("div");
    row.className = "upload-row";
    row.innerHTML = `<strong></strong><span></span>`;
    row.querySelector("strong").textContent = file.name;
    row.querySelector("span").textContent = t("uploading");
    elements.uploadList.prepend(row);

    try {
      const metadata = await api(`/api/books?filename=${encodeURIComponent(file.name)}`, {
        method: "POST",
        headers: {
          "content-type": "application/epub+zip"
        },
        body: file
      });
      row.querySelector("span").textContent = t("ready");
      state.selectedBookId = metadata.book_id;
      await refreshAll();
      showToast(`${metadata.title} ${t("isReady")}`);
    } catch (error) {
      row.querySelector("span").textContent = `${t("failed")}: ${error.message}`;
    }
  }
}

elements.loginForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  elements.loginError.textContent = "";
  try {
    const session = await api("/api/session", {
      method: "POST",
      body: JSON.stringify({ password: elements.password.value })
    });
    state.csrf = session.csrf;
    state.devNoAuth = Boolean(session.dev_no_auth);
    state.authMode = session.auth_mode ?? (state.devNoAuth ? "development" : "password");
    state.authenticated = Boolean(session.authenticated);
    updateAuthButton();
    updateLoginMode();
    updateAuthSummary();
    updateProtectedControls();
    elements.loginDialog.close();
    await refreshAll();
  } catch (error) {
    elements.loginError.textContent = error.message;
  }
});

elements.detailForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const book = selectedBook();
  if (!book) {
    return;
  }
  try {
    await api(`/api/books/${book.book_id}/metadata`, {
      method: "PUT",
      body: JSON.stringify({
        title: elements.detailTitle.value,
        authors: elements.detailAuthors.value.split(",").map((value) => value.trim()).filter(Boolean),
        language: elements.detailLanguage.value
      })
    });
    await refreshAll();
    showToast(t("metadataSaved"));
  } catch (error) {
    showToast(error.message);
  }
});

elements.authButton.addEventListener("click", async () => {
  if (state.devNoAuth) {
    showToast(t("devAuthNotice"));
    return;
  }
  if (!state.authenticated) {
    elements.loginError.textContent = "";
    elements.loginDialog.showModal();
    return;
  }
  try {
    await api("/api/session", { method: "DELETE" });
    state.authenticated = false;
    state.csrf = "";
    updateAuthButton();
    updateAuthSummary();
    updateProtectedControls();
    state.library = null;
    state.diagnostics = null;
    state.deviceTokens = [];
    state.selectedBookId = null;
    renderLibrary();
    renderSelectedBook();
    showToast(t("signedOut"));
  } catch (error) {
    showToast(error.message);
  }
});

elements.extractCover.addEventListener("click", async () => {
  const book = selectedBook();
  if (!state.authenticated) {
    showToast(t("signInToView"));
    return;
  }
  if (!book) {
    return;
  }

  try {
    showToast(t("extractingCover"));
    await api(`/api/books/${book.book_id}/cover/extract`, { method: "POST" });
    await refreshAll();
    showToast(t("extractedCover"));
  } catch (error) {
    showToast(error.message.includes("cover not found") ? t("noEmbeddedCover") : error.message);
  }
});

elements.deleteBook.addEventListener("click", async () => {
  const book = selectedBook();
  if (!state.authenticated) {
    showToast(t("signInToView"));
    return;
  }
  if (!book || !window.confirm(`${t("deleteConfirm")}\n\n${book.title || "Untitled"}`)) {
    return;
  }

  try {
    await api(`/api/books/${book.book_id}`, { method: "DELETE" });
    state.selectedBookId = null;
    await refreshAll();
    showToast(t("deletedBook"));
  } catch (error) {
    showToast(`${t("deleteFailed")}: ${error.message}`);
  }
});

elements.resetProgress.addEventListener("click", async () => {
  const book = selectedBook();
  if (!state.authenticated) {
    showToast(t("signInToView"));
    return;
  }
  if (!book || !window.confirm(`${t("resetProgressConfirm")}\n\n${book.title || "Untitled"}`)) {
    return;
  }

  try {
    await api(`/api/progress/${book.book_id}`, { method: "DELETE" });
    await refreshLibrary({ preserveDetail: true });
    showToast(t("progressReset"));
  } catch (error) {
    showToast(`${t("progressResetFailed")}: ${error.message}`);
  }
});

elements.coverInput.addEventListener("change", async () => {
  const book = selectedBook();
  const file = elements.coverInput.files?.[0];
  if (!state.authenticated) {
    showToast(t("signInToView"));
    elements.coverInput.value = "";
    return;
  }
  if (!book || !file) {
    return;
  }
  const contentType = imageContentType(file);
  if (!contentType) {
    showToast(t("coverUploadFailed"));
    elements.coverInput.value = "";
    return;
  }

  try {
    showToast(t("coverUploading"));
    await api(`/api/books/${book.book_id}/cover`, {
      method: "POST",
      headers: {
        "content-type": contentType
      },
      body: file
    });
    elements.coverInput.value = "";
    await refreshAll();
    showToast(t("coverUploaded"));
  } catch (error) {
    elements.coverInput.value = "";
    showToast(`${t("coverUploadFailed")}: ${error.message}`);
  }
});

elements.generateDeviceToken.addEventListener("click", () => {
  if (!state.authenticated) {
    showToast(t("signInToView"));
    return;
  }
  elements.deviceTokenError.textContent = "";
  elements.deviceName.value = "";
  elements.deviceTokenOutput.value = "";
  elements.deviceTokenForm.classList.remove("hidden");
  elements.deviceTokenResult.classList.add("hidden");
  elements.deviceTokenDialog.showModal();
  elements.deviceName.focus();
});

elements.deviceTokenForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  elements.deviceTokenError.textContent = "";
  try {
    const created = await api("/api/device-tokens", {
      method: "POST",
      body: JSON.stringify({ name: elements.deviceName.value })
    });
    elements.deviceTokenOutput.value = JSON.stringify({
      server_url: window.location.origin,
      device_token: created.token
    }, null, 2);
    elements.deviceTokenForm.classList.add("hidden");
    elements.deviceTokenResult.classList.remove("hidden");
    await refreshDeviceTokens();
    showToast(t("tokenCreated"));
  } catch (error) {
    elements.deviceTokenError.textContent = error.message;
  }
});

document.querySelector("#cancel-device-token").addEventListener("click", () => {
  elements.deviceTokenDialog.close();
});

document.querySelector("#close-device-token").addEventListener("click", () => {
  elements.deviceTokenDialog.close();
});

document.querySelector("#copy-device-token").addEventListener("click", async () => {
  const pairingData = elements.deviceTokenOutput.value;
  try {
    await navigator.clipboard.writeText(pairingData);
  } catch {
    elements.deviceTokenOutput.select();
    document.execCommand("copy");
  }
  showToast(t("pairingDataCopied"));
});

elements.deviceTokenDialog.addEventListener("close", () => {
  elements.deviceTokenOutput.value = "";
  elements.deviceTokenError.textContent = "";
});

elements.language.addEventListener("change", () => {
  saveSettings({ language: elements.language.value }, elements.settingsStatus);
});

elements.defaultSort.addEventListener("change", () => {
  saveSettings({ library_sort: elements.defaultSort.value }, elements.settingsStatus);
});

elements.downloadPackage.addEventListener("click", () => {
  const book = selectedBook();
  if (!state.authenticated) {
    showToast(t("signInToView"));
    return;
  }
  if (book) {
    window.location.href = `/api/books/${book.book_id}/reader-package`;
  }
});

elements.fileInput.addEventListener("change", () => {
  uploadFiles(elements.fileInput.files);
  elements.fileInput.value = "";
});
elements.detailClose.addEventListener("click", () => closeDetails());
elements.detailBackdrop.addEventListener("click", () => closeDetails());
elements.search.addEventListener("input", () => {
  renderLibrary();
  renderSearchResults();
});
elements.search.addEventListener("focus", renderSearchResults);
elements.sort.addEventListener("change", () => {
  renderLibrary();
  saveSettings({ library_sort: elements.sort.value }, elements.settingsStatus);
});

document.addEventListener("click", (event) => {
  if (!elements.searchResults.contains(event.target) && event.target !== elements.search) {
    elements.searchResults.classList.add("hidden");
  }
});

document.addEventListener("keydown", (event) => {
  if (event.key === "Escape" && state.selectedBookId && !document.querySelector("dialog[open]")) {
    event.preventDefault();
    closeDetails();
  }
});

document.querySelectorAll(".nav-item").forEach((button) => {
  button.addEventListener("click", () => switchView(button.dataset.view));
});

document.querySelectorAll("[data-library-mode]").forEach((button) => {
  button.addEventListener("click", () => {
    state.libraryMode = button.dataset.libraryMode;
    document.querySelectorAll("[data-library-mode]").forEach((item) => item.classList.toggle("active", item === button));
    renderLibrary();
    saveSettings({ library_view: state.libraryMode }, elements.settingsStatus);
  });
});

for (const eventName of ["dragenter", "dragover"]) {
  elements.libraryView.addEventListener(eventName, (event) => {
    event.preventDefault();
    if (state.authenticated) {
      elements.libraryView.classList.add("dragover");
    }
  });
}

for (const eventName of ["dragleave", "drop"]) {
  elements.libraryView.addEventListener(eventName, (event) => {
    event.preventDefault();
    elements.libraryView.classList.remove("dragover");
  });
}

elements.libraryView.addEventListener("drop", (event) => {
  if (!state.authenticated) {
    showToast(t("uploadRequiresSignIn"));
    return;
  }
  uploadFiles(event.dataTransfer.files);
});

window.matchMedia("(prefers-color-scheme: dark)").addEventListener("change", () => {
  if (state.settings.display_theme === "system") {
    applyTheme();
  }
});

window.addEventListener("focus", refreshExternalState);
document.addEventListener("visibilitychange", () => {
  if (document.visibilityState === "visible") {
    refreshExternalState();
  }
});

applySettings(state.settings);
applyTranslations();
refreshSession()
  .then(() => {
    if (!state.authenticated) {
      setApiStatus("bad", "signInRequired");
      renderLibrary();
      renderSelectedBook();
      return null;
    }
    return refreshAll();
  })
  .catch((error) => {
    state.authenticated = false;
    state.csrf = "";
    updateAuthButton();
    updateLoginMode();
    updateAuthSummary();
    updateProtectedControls();
    renderLibrary();
    renderSelectedBook();
    setApiStatus("bad", error.message === t("authenticationRequired") ? "signInRequired" : "apiUnavailable");
    showToast(error.message);
  });

window.setInterval(refreshExternalState, externalRefreshIntervalMs);
