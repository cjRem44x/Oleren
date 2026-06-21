// Register 'olrn' as a highlight.js alias for Rust — closest grammar match.
// Runs after highlight.js and book.js are loaded.
(function () {
    if (typeof hljs === 'undefined') return;
    hljs.registerAliases(['olrn'], { languageName: 'rust' });

    // Re-highlight any olrn blocks that were skipped (no-language fallback).
    document.querySelectorAll('code.language-olrn').forEach(function (block) {
        hljs.highlightElement(block);
    });
})();
