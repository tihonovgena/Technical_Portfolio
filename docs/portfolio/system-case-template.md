# System Case Template

Use this template for every new portfolio system. The goal is to make the system reviewable in 2-3 minutes by a technical lead.

---

# System Name

One-sentence summary of what the system does and why it exists.

## Status

- **Current status:** Prototype / In progress / Stable enough for prototype use
- **Main purpose:** What gameplay or production problem this system solves
- **Best files to review first:** Link 2-4 files only

## Problem

Describe the concrete problem that forced this system to exist.

Good examples:
- "The player needs to drag replicated world objects with responsive local feedback."
- "The game needs nearby object markers without scanning every actor every frame."

## Constraints

List the constraints that shaped the implementation.

Examples:
- multiplayer authority
- local responsiveness
- designer-facing workflow
- prototype speed
- avoiding full game-source exposure
- performance limits

## Architecture

```text
System diagram here
```

Explain the main objects and responsibilities.

## Networking / Authority Model

Use this section only if the system is networked.

Cover:
- what runs on the client
- what runs on the server
- what is predicted locally
- what is replicated
- how reconciliation or correction works

## Implementation Notes

Explain the most important implementation decisions.

Avoid describing every function. Focus on decisions.

## Trade-offs

Explain what was intentionally simplified or postponed.

This is important for senior-level review because it shows that the code was written with constraints, not in a vacuum.

## Known Limitations

List current limitations honestly.

## Next Steps

List realistic next improvements.

