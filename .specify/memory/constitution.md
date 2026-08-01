# Minesweeper Constitution

> This is the template's starting-point constitution, carried over from a
> real Kobo game built with this architecture. Its six principles are
> already generic to "a Kobo device app" — review and tighten the wording
> (especially Principle III's examples) for your actual project before your
> first `/speckit-plan`.

## Core Principles

### I. Portable Core, Thin Platform Layer

All app/game logic — rules, state, serialization, and screen/widget layout
— MUST live in a portable core with no OS, filesystem, rendering, or input
calls. Device- and host-specific code is confined to `src/platform/` behind
the `Renderer` and `TouchInput` interfaces, and to a thin persistence
adapter. The core MUST compile and run on the development host without any
Kobo hardware or toolchain.

Rationale: this is what makes the app testable on a PC, developable without
flashing the device, and portable across the Kobo lineup — the three
riskiest goals of this class of project.

### II. E-ink-First, Grayscale-First UX

The UI MUST be designed for e-ink: partial refreshes for in-session updates,
full refreshes on screen transitions and periodically to clear ghosting, and
no UI element that requires frequent redraws (an always-ticking seconds
timer is forbidden; minute-level updates are the ceiling). Every piece of
information MUST be distinguishable in pure grayscale — color is accent
only, never the sole carrier of meaning. Touch targets MUST scale from
display DPI, never hardcoded pixels.

Rationale: the product ships on monochrome and color e-ink devices of
several sizes; a UI that assumes LCD behavior or color semantics fails
portability and usability.

### III. Host-Testable Correctness (NON-NEGOTIABLE)

Correctness-critical core logic MUST have host-run unit tests that pass
before a change merges: app/game state transitions, any generative or
procedural logic (uniqueness/validity properties, tested across many
inputs), and persistence (round-trip fidelity and corrupted-input
recovery). A state that fails its own invariants, or a save that cannot
survive a round-trip, is a release-blocking defect.

Rationale: on-device debugging is slow and crude (a crash.log at best);
correctness must be established where tests can actually run.

### IV. Firmware-Agnostic Device Integration

The app MUST NOT link against or depend on Kobo's private firmware
libraries (libnickel/Qt internals) or any firmware-version-specific
behavior. Device integration is limited to stable, community-proven
surfaces: the e-ink framebuffer via FBInk, evdev input, and launch via
KFMon and/or NickelMenu configs. Either launch mechanism MAY be documented
as a feature's primary, recommended path, provided a
firmware-4.x/5.x-compatible mechanism (KFMon, today) always remains
available and documented for devices where NickelMenu does not apply —
NickelMenu MUST NOT become the only supported launch path, since it does
not work on firmware 5.x. One armhf binary MUST serve all supported
devices; installation MUST remain plain file copy over USB.

Rationale: private-API dependencies have broken entire NickelMenu-based
ecosystems on firmware 5.x; avoiding them is the difference between "works
on my device today" and "works on other Kobos across firmware updates".

### V. Never Lose the User's Progress

App state MUST be persisted after every mutating action using atomic
write-then-rename. Unreadable or invalid persistent files MUST degrade to
defaults without crashing — a corrupt save costs at most the current
session's unsaved progress, never a crash loop. The app MUST persist and
exit cleanly on termination signals so device sleep/power events cannot
corrupt state.

Rationale: reliability defines user trust for a casual e-reader app;
violating it makes the app feel broken regardless of features.

### VI. Simplicity and Minimal Dependencies

Prefer the simplest design that satisfies the spec. New third-party
dependencies MUST be vendored, MUST build with the koxtoolchain
cross-compiler, and MUST be justified in writing (in plan.md or a PR
description) against the alternative of writing the small amount of code
directly. Header-only libraries are preferred. No frameworks, no
speculative abstractions beyond the two platform interfaces of Principle I.

Rationale: the target is a single small binary on a constrained device;
every dependency is cross-compilation risk and binary-size cost.

## Technology Constraints

- Language: C++17. Device builds use koxtoolchain (`arm-kobo-linux-gnueabihf`) under WSL2/Docker.
- Rendering: FBInk (vendored). Input: raw evdev. Launcher: KFMon (required — the
  firmware-4.x/5.x-compatible fallback) and NickelMenu (approved, firmware-4.x only); either
  may be documented as a feature's primary launch path per Principle IV.
- Persistence: JSON via nlohmann/json (vendored, header-only) in `.adds/<app>/`.
- Tests: doctest (vendored, header-only), built and run on the host.
- Approved dependency set is exactly the above plus SDL2 (host-only simulator); additions go
  through Principle VI justification.

## Development Workflow & Quality Gates

- Every feature follows the speckit flow: spec → clarify → plan → tasks → implement; the plan's
  Constitution Check gate MUST evaluate Principles I–VI explicitly.
- Task generation MUST include the Principle III test tasks for any work touching core logic;
  tests for a story are written with (or before) the implementation, never deferred past it.
- A change is "done" only when host tests pass and, for user-visible behavior, the relevant
  quickstart.md validation scenario has been run (on device or simulator, as the scenario
  specifies).
- Layout and rendering changes MUST be sanity-checked in grayscale (Principle II) before being
  considered complete.

## Governance

This constitution supersedes ad-hoc practice for this repository. Amendments are made by
editing `.specify/memory/constitution.md` with an updated Sync Impact Report, a semantic
version bump, and propagation to dependent templates and the active feature's plan.

Versioning policy: MAJOR for removing or redefining a principle in a backward-incompatible
way; MINOR for adding a principle or materially expanding guidance; PATCH for clarifications
and wording. Compliance is reviewed at every `/speckit-plan` Constitution Check and re-checked
after design; violations must be justified in the plan's Complexity Tracking table or the
design changed.

**Version**: 1.0.0 | **Ratified**: 2026-07-25 | **Last Amended**: 2026-07-25
