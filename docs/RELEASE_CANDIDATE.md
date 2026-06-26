# Release-candidate track

`configlib` v0.12 marked the release-candidate preparation track. v0.13.0 begins the fire-testing phase before the eventual v1.0 stable release.

The purpose of this stage is not to add new features. The purpose is to make the public surface boring, supportable, documented, installable, and ready to be attacked by the fire tests.

## Release-candidate rule

After v0.12, new work intended for the v1 public surface should be additive unless fire testing proves that a correctness, safety, security, or serious usability defect must be fixed before v1.0.

Allowed after this point:

- Add tests.
- Add diagnostics.
- Add documentation.
- Add examples.
- Add tooling around the library.
- Add optional behavior behind explicit policy.
- Add new API without changing existing API contracts.

Not allowed without an explicit ruling:

- Rename public C++ symbols.
- Rename public C ABI functions, status codes, or handle types.
- Change getter semantics.
- Change default handling.
- Change conflict-resolution behavior silently.
- Change export/redaction behavior silently.
- Change C ABI ownership or borrowed-string lifetime rules.
- Remove public functions.
- Make previously valid public calls invalid without a documented migration reason.

## Exception rule

The additive-only rule is not a suicide pact.

If the v0.13 fire tests prove that an existing public contract is unsafe, ambiguous, corrupting, misleading, or impossible to support honestly in v1, then the defect must be fixed before v1.0.

Any such exception must be documented in:

- `docs/CHANGELOG.md`
- `docs/API_STABILITY.md`
- `docs/API_FREEZE_CANDIDATE.md` if it changes the freeze ruling
- migration notes if existing users must change code

## v1 release-candidate checklist

Before v1.0, verify the following from a fresh source archive.

### Build and install

- Clean configure with CMake/Ninja.
- Static library builds.
- Shared library builds.
- Examples build and run.
- Tests pass.
- Install target installs headers, libraries, CMake package files, and pkg-config metadata.
- External CMake consumer can use `find_package(configlib CONFIG REQUIRED)`.
- External C consumer can use `pkg-config --cflags --libs configlib`.

### Public API

- C++ public headers are reviewed.
- C ABI header is reviewed.
- Naming follows the documented getter/setter/fallback rules.
- Defaults are facts, not policy-synthesized hidden values.
- `get_*()` returns optional where absence/type mismatch can occur.
- `get_*_or(...)` is the only fallback spelling.
- Opaque C handles have documented ownership rules.
- Borrowed strings have documented lifetimes.

### Behavior

- Same-priority conflicts are diagnosed.
- Rejections have reasons.
- Schema validation reports missing, mistyped, and invalid values.
- Runtime transactions fail without corrupting store state.
- Views cannot escape their prefix.
- Export modes respect `exportable=false` and secret redaction.
- No public operation silently guesses where policy is required.

### Documentation

- README introduces the project honestly.
- HOWTO explains build, install, use, and binding-language entry points.
- Programmer's manual is aligned with current API.
- C ABI manual matches `configlib.h`.
- API stability and freeze documents match actual behavior.
- Lifetimes document matches C++ and C ABI ownership.
- Terminology and explained-simply documents remain accurate.
- Packaging document matches install output.

### Fire-test readiness

- `docs/FIRE_TESTING.md` is the active v0.13 test plan.
- Sanitizer instructions are current.
- Fuzzing candidates are identified.
- Edge-case categories are listed.
- Security boundaries are explicit.

## v1 gate question

The v1 decision should not be based on whether the happy path works.

The v1 decision should be based on this:

> Can configlib be trusted when users are wrong, files are broken, bindings are clumsy, and hostile inputs are annoying?

If the answer is yes, v1 is earned.
