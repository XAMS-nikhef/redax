# REDAX Code Quality Issues

Analysis date: 2026-05-21

---

## Executive Summary

The codebase is a C++ DAQ system of mixed quality. It adopts some modern C++17 idioms (smart pointers, structured bindings, atomics) but has serious correctness issues in thread safety, memory management, and error handling that are likely to surface under real hardware conditions.

---

## 1. Memory Management

**Good:** `std::shared_ptr`/`std::unique_ptr` used throughout `DAQController`, `StraxFormatter`, `Options`. Move semantics implemented on `data_packet`.

**Critical issues:**

- **V1724.cc** — raw `new char32_t[]` allocations inside the readout loop. If `s.append()` throws after the pointer enters `xfer_buffers`, cleanup is correct, but the path is brittle. A `std::vector<std::unique_ptr<char32_t[]>>` would be safe by default.
- **V1724.cc** — `GetClockInfo()` returns `{0xFFFFFFFF, -1}` on failure, but the caller constructs a `data_packet` with those sentinel values without validation.
- **MongoLog.cc** — holds a `mongocxx::pool::entry` for its entire lifetime, risking pool exhaustion if multiple instances exist.

---

## 2. Thread Safety

This is the most serious area.

- **`volatile std::atomic_long fDataRate`** — DAQController.hh. `volatile` and `atomic` are incompatible concepts; this indicates a misunderstanding of C++ concurrency. `volatile` does not provide synchronization and should be removed.
- **`fDigitizers` map race** — modified in `ReadData()`, read concurrently in `StatusUpdate()` (called from a separate thread in main.cc) with no mutex held on the reading side. This is a data race and undefined behaviour.
- **Clock counter race** — V1724.cc `fLastClock` and `fRolloverCounter` are written without a lock but read from multiple processing threads via `StraxFormatter`. Classic TOCTOU.
- **Busy-wait in `ReceiveDatapackets`** — StraxFormatter.cc uses `try_lock()` and returns 1 on failure; the caller in DAQController.cc retries in a tight loop. This wastes CPU and can stall if all formatter threads happen to be busy simultaneously.
- **`f1724::sRegistry` unprotected** — f1724.hh declares `static std::atomic_bool sRun/sReady` (correctly atomic) but `static std::vector<f1724*> sRegistry` with no mutex. `emplace_back` and `GlobalRun` reads can race.

---

## 3. Error Handling

- **`ReadRegister()` returns `0xFFFFFFFF` on error** — callers cannot distinguish this from a legitimate register value. None of the callers check for this sentinel.
- **File write errors are silently ignored** — StraxFormatter.cc: `writefile.write(...)` and `writefile.close()` have no error checks. Lost data would go undetected.
- **Exceptions in destructors** — V1724.cc `~V1724()` calls `End()` and `fLog->Entry()`, both of which can throw. Throwing from a destructor during stack unwinding terminates the program.
- **`Arm()` failure does not halt the state machine** — main.cc logs the error and calls `Stop()` but does not `return`, so the process continues polling as if everything is fine.
- **BSON field access without `try/catch`** — main.cc: individual `.get_string()` calls on incoming documents will throw on missing or wrong-typed fields; the outer catch is broad and swallows context.

---

## 4. Resource Management

- **Hardware handle not validated before use** — `fBoardHandle` is set to `-1` on failed init, but subsequent calls to `GetAcquisitionStatus()` → `ReadRegister()` pass it to the CAEN library without checking.
- **Partial initialisation cleanup is too aggressive** — DAQController.cc: on any board `Init()` failure the entire `fDigitizers` map is cleared, including boards that initialised successfully.
- **`MongoLog` file handle left in bad state** — MongoLog.cc: if `fOutfile.open()` fails, the function returns `-1` but the `std::ofstream` object remains open/closed inconsistently for future calls.

---

## 5. Code Structure

- **`DAQController` is a God class** — handles digitizer lifecycle, readout threads, status reporting, and acquisition control in one class.
- **`StraxFormatter` has too many responsibilities** — compression (LZ4, Blosc), file I/O, event processing, fragment assembly, and data unpacking all in one class. These should be separated.
- **`Options` is a kitchen sink** — 20+ untyped `Get*` methods returning `int`/`double`/`string` with defaults. Type-safe configuration structs would eliminate entire categories of silent misconfiguration bugs.
- **Circular headers** — `DAQController` ↔ `V1724` ↔ `StraxFormatter` ↔ `Options` are tightly coupled, making unit testing and refactoring hard.

---

## 6. C++ Modernness

**Good:** structured bindings, `std::atomic`, `std::thread`, smart pointers.

**Issues:**
- Raw `new`/`delete[]` in hot readout path (V1724.cc) instead of `std::vector` or `std::make_unique`.
- C-style variadic `va_list`/`vsnprintf` in MongoLog.cc — should use `std::format` (C++20) or at minimum `std::ostringstream`.
- Manual date string formatting in MongoLog.cc instead of `<chrono>`.
- No `const` on any getter methods (`status()`, `bid()`, `link()`, etc.).
- No `noexcept` on functions that demonstrably cannot throw.
- Commented-out deprecated `CAENVME_Init` call left in V1724.cc.

---

## 7. Security

- **Buffer over-read** — StraxFormatter.cc: `fragment.data()+14` and the pointer cast to `short*` have no length check. Malformed hardware data could read out of bounds.
- **Integer overflow** — V1724.cc: `(rollovers<<31) + ch_time` — if `rollovers` is large, the left-shift overflows a 32-bit integer (undefined behaviour in C++).
- **No schema validation on MongoDB documents** — main.cc: the `options_override` JSON from the database is passed directly to the `Options` constructor. A compromised or misconfigured database entry could inject arbitrary option values.

---

## 8. Bug Tracker

| Severity | File | Description |
|---|---|---|
| 🔴 Critical | DAQController.hh | `volatile std::atomic_long` — wrong semantics, remove `volatile` |
| 🔴 Critical | DAQController.cc / main.cc | `fDigitizers` data race between `ReadData` and `StatusUpdate` threads |
| 🔴 Critical | V1724.cc | Clock counters `fLastClock`/`fRolloverCounter` not thread-safe |
| 🟠 High | V1724.cc | `ReadRegister()` error sentinel `0xFFFFFFFF` not checked by callers |
| 🟠 High | StraxFormatter.cc | `data()+14` pointer arithmetic without bounds check |
| 🟠 High | V1724.cc | `(rollovers<<31)` — signed integer overflow, undefined behaviour |
| 🟠 High | StraxFormatter.cc | File write errors silently ignored |
| 🟠 High | f1724.hh | `sRegistry` vector accessed without lock |
| 🟡 Medium | V1724.cc | `~V1724()` can throw (called from destructor during stack unwinding) |
| 🟡 Medium | V1724.cc | Division by zero if `cal_values["slope"][ch] == 0` in `ClampDACValues` |
| 🟡 Medium | CControl_Handler.cc | `fDDC10` only initialised under `#ifdef HASDDC10`, used unconditionally |
| 🟢 Low | V1724.cc | Commented-out deprecated `CAENVME_Init` API call |

---

## 9. File-by-File Assessment

| File | Quality | Notes |
|---|---|---|
| main.cc | Poor | Missing error handling, no graceful shutdown |
| DAQController.cc | Fair | Race conditions, resource leaks, complex logic |
| DAQController.hh | Fair | Missing `const`, `volatile`+atomic misuse |
| V1724.cc | Fair | Memory management, error returns, thread-unsafe |
| V1724.hh | Good | Clear API, but missing `const` |
| StraxFormatter.cc | Poor | Buffer overflows, error handling, bounds checking |
| StraxFormatter.hh | Fair | God object pattern |
| MongoLog.cc | Fair | Resource cleanup, C-style variadic functions |
| MongoLog.hh | Good | Clear interface |
| Options.cc | Fair | No type safety, exception handling in `Load()` |
| Options.hh | Fair | Too many responsibilities |
| CControl_Handler.cc | Fair | Incomplete initialisation |
| f1724.cc | Fair | Static state management, initialisation complexity |
| f1724.hh | Fair | Complex static member management |

---

## 10. Priority Fix List

### 🔴 Critical — fix first
1. Add a mutex protecting `fDigitizers` — every access site in both the `ReadData` thread and the `StatusUpdate` thread must hold it.
2. Remove `volatile` from `fDataRate` in DAQController.hh.
3. Replace raw `new[]` in V1724.cc with `std::vector<uint8_t>` or `std::make_unique<char32_t[]>`.
4. Guard `fLastClock`/`fRolloverCounter` with a per-digitizer mutex in V1724.cc.

### 🟠 High — fix second
1. Return `std::optional<uint32_t>` from `ReadRegister()` to force callers to handle failure.
2. Add bounds check before the `data()+14` pointer cast in StraxFormatter.cc.
3. Fix signed shift overflow `rollovers<<31` — cast to `uint64_t` before shifting.
4. Add error checks on `writefile.write()` and `writefile.close()` in StraxFormatter.cc.
5. Protect `f1724::sRegistry` with a `std::mutex`.

### 🟡 Medium — fix third
1. Mark `~V1724()` operations as `noexcept` and handle errors without throwing.
2. Add a zero-check before dividing by `cal_values["slope"][ch]` in `ClampDACValues`.
3. Ensure `fDDC10` has a consistent initialisation path regardless of `HASDDC10`.
4. Make all getter methods `const`.

### 🟢 Low — nice to have
1. Remove commented-out deprecated `CAENVME_Init` call in V1724.cc.
2. Replace `va_list`/`vsnprintf` in MongoLog.cc with `std::ostringstream`.
3. Replace manual date formatting with `<chrono>`.
4. Refactor `StraxFormatter` into smaller, single-responsibility classes.
5. Convert `Options` to type-safe configuration structs.
