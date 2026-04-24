# Documentation

This documentation is the entry point for understanding and integrating `voxel-engine`.
It is organized around three simple paths: using the library, understanding the architecture, and working on the engine internals.

## I Want To Use The Engine

Start here if you are integrating `voxel-engine` into a client application.

- [Public API](public-api.md)
- [Configuration](configuration.md)
- [Build and Integration](build.md)

## I Want To Understand How It Works

Start here if you want the architectural view first.

- [Architecture Overview](architecture-overview.md)
- [Runtime Flow](runtime-flow.md)
- `design-decisions.md` (planned)

## I Want To Work On The Engine

Start here if you are reading or changing the implementation.

- [World Layer](world-layer.md)
- [Engine Layer](engine-layer.md)
- [Graphics Layer](graphics-layer.md)
- [Development Notes](development-notes.md)

## Documentation Structure

- `README.md`
  - repository overview
- `docs/public-api.md`
  - user-facing engine facade
- `docs/configuration.md`
  - initialization and lifetime rules
- `docs/architecture-overview.md`
  - top-level structure
- `docs/runtime-flow.md`
  - init, update, render, shutdown, and swapchain recreation
- `docs/world-layer.md`, `docs/engine-layer.md`, `docs/graphics-layer.md`
  - internal layer references
