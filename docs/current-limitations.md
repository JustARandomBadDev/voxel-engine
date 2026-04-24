# Current Limitations

This page lists the main limitations of the engine in its current state.
These are not design goals. They are simply the current boundaries of the project.

## Rendering Backend

- **Vulkan-specific backend**
  - The graphics layer is currently written directly against Vulkan.
- **No renderer abstraction**
  - The engine does not currently provide a backend-agnostic rendering interface.

## Runtime and Systems

- **No multithreading**
  - World updates, meshing, synchronization, and rendering are currently driven in a straightforward single-threaded flow.
- **No built-in streaming system**
  - Dynamic loading and unloading policy is not an engine-level system at this stage.
- **No LOD system**
  - Chunk rendering currently assumes a single geometry representation per chunk.

## Performance and Optimization

- **Optimization is still limited**
  - The engine is functional, but performance work is not yet the main focus.
- **No advanced scheduling**
  - There is no job system or task orchestration layer.
- **Some graphics-side lifecycle details are still evolving**
  - The graphics runtime is cleaner than before, but some internal contracts and lifecycle rules are still being stabilized.

## Public API

- **Public API is still evolving**
  - The facade is intentionally small, but it should not yet be treated as a finalized long-term external API.
- **Initialization is Vulkan-oriented**
  - Client integration currently exposes Vulkan-specific host configuration in the init contract.

## Scope

- **Not a full game engine**
  - The project provides a voxel engine core, not a complete gameplay, editor, tooling, or content pipeline stack.
