# Release Notes

## v1.0.1 - 2026-03-01

### Miglioramenti principali

- Rimosso completamente il supporto buzzer dal firmware.
- Consolidato il progetto in un unico sorgente: `src/main.cpp`.
- Snellita la struttura del codice per facilitare manutenzione e debug.

### Modifiche tecniche

- Eliminati i file separati del firmware e unificata la logica in `src/main.cpp`.
- Aggiornati i riferimenti interni per mantenere la build stabile con file unico.
- README aggiornato con struttura progetto, comandi build/upload e schema rapido collegamenti.

### Verifica

- Build PlatformIO eseguita con successo su ambiente `megaatmega2560`.

### Note compatibilità

- Progetto in sviluppo attivo.
- Nessuna dipendenza nuova introdotta.
