# Scripting typings (server)

IntelliSense and reference docs for Mafia Connected **server-side** scripts. The client-side
equivalent is in the MafiaC repo, under `Typings/`.

| File | Use |
| --- | --- |
| `mafiac-server.d.ts` | TypeScript declarations with JSDoc comments. VSCode reads these for autocomplete, hover docs and signature help in plain `.js` scripts. |
| `mafiac-server.jsdoc.js` | The same API as JSDoc doclets, for building an HTML reference with [`jsdoc`](https://jsdoc.app/) (`npx jsdoc mafiac-server.jsdoc.js -d html`). It holds only comments. Don't include it in an editor project. |

## Using it in VSCode

Add a `jsconfig.json` next to the server scripts:

```json
{
	"compilerOptions": { "target": "es2020", "lib": ["es2020"] },
	"include": ["**/*.js", "../typings/mafiac-server.d.ts"]
}
```

- **`"lib": ["es2020"]` is required.** Without it, VSCode loads the browser DOM types, and their
  `Event` and `Element` collide with the engine's classes of the same names.
- **Keep client and server scripts in separate folders, each with its own `jsconfig.json`.** The
  two sides declare the same class and function names with different members.

## Regenerating

These files are generated, so don't edit them by hand. The generator and full instructions are in
the MafiaC repo (`Tools/Typings/generate_typings.py`, `Typings/README.md`). In short: run `dumpdoc`
in the server console with no resources running, then, from the MafiaC repo root:

```sh
python3 Tools/Typings/generate_typings.py --side server \
    --dump <server dir>/Documentation \
    --source ../MafiaC-Server/Server --source ../Galactic/src \
    --out ../MafiaC-Server/Typings/mafiac-server
```
