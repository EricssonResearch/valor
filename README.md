## Persisting C++ coroutines with Valor

Suspended coroutines (*coro*s) can be resumed in the same thread, or moved to another one, but not across processes. In generel, this isn't possible even theoretically, due to user-defined types and those in 3rd party libraries.
What we try here instead is to find a compromise between the restrictions under which moving a suspended coro to another process is still possible such that these restrictions allow a generic enough class of source code to be written.

The system in short works by injecting *source level* augmentation to user code that serializes *local variables* of coros when suspended, and deserializing them before continuing from a suspension point. This allows a broad set of C++ types including STL containers to be used in these coros, and export/import their internal state between OS processes.

The system we propose comprises

- a header file `valor.h` that should be included by user code, defining the special coro handle type that user functions should return;
- a source code rewriting tool `valet` that injects the calls for serialization and deserialization of local variables;
- and the implicitly included serialization library `ufser` that defines what types can be handled by Valor.

The augmented source code is then can be further processed with any C++-20 standards compliant compiler---the intention of Valor is at least to only use standard language constructs.[^1]

[^1]: GCC in this regard is more forgiving than Clang, most notably with respect to jumping over variable declarations---which we *know* will get initialized but can't explain the guy.

### Usage

Write code, e.g., `mycode.cc` with coros you want to save returning `valor::ser_ctx`. These are always initial-suspended. When any of such coros are suspended, you can call `ser_ctx::serialize()` on their handles, and save or transport the resulting string to another process. There you can call the coro (returning an initially suspended version), and use `ser_ctx::deserialize()`, then trigger a continuation.
Some helpers are provided to save typing starting/restarting parts. Run the source first with Valet, producing `mycode_valet.cc`, then compile that as usual.

An example for the workflow can be found in `valor.cc` as run by `make check`. Function `k` has some suspension points that are reported as `k()` progresses. After the first, the function's state is copied over to a different process by writing the serialized string to stdin. The second process then continues where the first left off. PIDs are shown on log lines for verification.

### Limitations

- Side effects during `co_await`
- Member coro:s
- Serializable types: ...
- Multiple co_await in a single full-expression (gcc bug 97452)

### Boost Fibers

We compare task switching overhead in Boost Fibers vs. coros in a simple tool. Two fibers/coros are used, called in an alternating order (for fibers, the default round-robin scheduler is hoped to do so; for coros we enforce it manually). The first, f1, simply yields to / waits for f2 in each iteration; the second, f2, counts down by one in each iteration then yields, starting from a number given on the cmdline. When f2 reaches zero, the program terminates. The total time of execution can be shown by running

```
make check -C balor
cat balor/many.log
```

where the time for fibers is shown first. NB: we run 8x more coro iterations to have roughly the same total time.

### Vapor

Clang AST not ideal --> coro proto is not parsed yet, symbolic calls and vars are still there which should be recreated in src code