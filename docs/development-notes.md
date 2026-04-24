# Development Notes

## Development Philosophy

The engine is developed with a small set of practical rules:

- incremental refactoring
- no global state
- explicit ownership
- simplicity over abstraction
- avoiding premature optimization

These are not abstract style preferences.
They are meant to keep the codebase understandable while it is still evolving.

## Incremental Refactoring

The codebase is already functional, so most changes are made as controlled refactors instead of large rewrites.

Why:

- it keeps behavior stable while the architecture is improved
- it makes regressions easier to isolate
- it avoids replacing working systems with speculative new designs

## No Global State

The engine avoids singletons and hidden global runtime objects.

Why:

- ownership stays visible
- dependencies are easier to follow
- initialization and shutdown remain explicit
- client integration is easier to reason about

## Explicit Ownership

Major systems are expected to own their resources directly and clean them up explicitly.

Why:

- this is especially important in the graphics runtime, where resource lifetime matters
- it makes cleanup and recreation paths easier to audit
- it reduces confusion about who creates, stores, and destroys a resource

## Simplicity Over Abstraction

The project prefers concrete systems over generic frameworks unless there is a clear need.

Why:

- the engine is still being shaped around real use cases
- generalization too early would make the code harder to understand
- local, concrete solutions are easier to maintain while the architecture is still settling

## Avoiding Premature Optimization

The engine does not treat every subsystem as a performance project from the start.

Why:

- correctness and clarity come first
- optimization is more effective once ownership and runtime flow are stable
- premature optimization would add complexity before the core contracts are fully settled

## Practical Direction

The current direction is to keep improving the engine by:

- clarifying internal contracts
- tightening ownership and lifecycle rules
- documenting the real architecture
- refining the public facade without overcommitting to a final API too early

This keeps the project moving forward without forcing it into a large theoretical design before the runtime model is fully mature.
