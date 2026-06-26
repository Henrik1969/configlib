# Lifetimes

`ConfigStore` owns configuration state.

`ConfigView` is a lightweight non-owning scoped view into a `ConfigStore`. A view must not outlive the store that created it.

`ConfigTransaction` is a lightweight non-owning mutation transaction bound to a `ConfigStore`. A transaction must not outlive its store.

Successful public factory/member operations must not return invalid handles. An empty view is valid if it was created by a valid store and merely has no matching keys.
