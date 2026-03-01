# Release Notes

## GitHub Release pronta

Tag: v1.0.2-cleanup

Titolo release: v1.0.2-cleanup - Cleanup firmware e documentazione

Descrizione:

- Cleanup generale completato con consolidamento del firmware in un solo file sorgente.
- Rimossi i moduli legacy non più utilizzati, incluso il buzzer.
- README aggiornato con schema rapido collegamenti e indicazioni operative.
- Versione firmware aggiornata a V1.01.
- Build verificata con successo su target megaatmega2560.

## v1.0.2-cleanup - 2026-03-01

### Miglioramenti principali

- Refactor di cleanup completato con struttura firmware semplificata.
- Allineata la documentazione operativa e hardware nel README.
- Versione firmware aggiornata a `V1.01`.

### Modifiche tecniche

- Consolidata l'architettura a file sorgente unico `src/main.cpp`.
- Rimozione definitiva dei moduli legacy non più utilizzati (incluso buzzer).
- Aggiunta documentazione di rilascio e schema rapido collegamenti.

### Verifica

- Build PlatformIO OK su `megaatmega2560`.

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
