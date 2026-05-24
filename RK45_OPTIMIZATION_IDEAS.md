# RK45 Integration Optimization Ideas

> **Status:** Under consideration — not yet implemented.  
> **Generated:** 2026-05-23 13:42 CST  
> **Context:** Speeding up the Runge–Kutta–Fehlberg (RK45) orbital propagation in `Simulation_Trajectory.cpp` without sacrificing physical accuracy.

---

## 1. Compiler Flags: `-march=native`

- **What:** The current `CMakeLists.txt` builds in `Release` mode (`-O3 -DNDEBUG`) but does **not** pass `-march=native`. Apple Clang therefore emits generic code and cannot use M4-specific instructions such as **FMA** (fused multiply-add).
- **Fix:** Add `-march=native` to `target_compile_options` for both the core library and the executable.
- **Impact:** 5–15% speedup on numerical code. Zero accuracy loss.

---

## 2. RK5 Local Extrapolation

- **What:** The current code advances the state with the **RK4** estimate (`radius = radius_4`, `v_radial = v_radial_4`, `phi = phi_4`). Standard practice for an embedded RK45 pair is to propagate with the **higher-order RK5** estimate and use the RK4–RK5 difference *only* for error estimation and step-size control.
- **Fix:** Change the state-update block to use `radius_5`, `v_radial_5`, `phi_5`.
- **Impact:** ~20–35% fewer RK45 steps for the same (or better) accuracy. Accuracy actually *improves*.
- **Location:** `vendor/damascus/src/Simulation_Trajectory.cpp` lines 666–671.

---

## 3. Replace `pow(x, 2.0)` and `pow(x, 3.0)` in Steffen Spline

- **What:** `libphysica::Interpolation::Interpolate()` uses `pow((x - x_j), 3.0)` and `pow((x - x_j), 2.0)`. The general `pow()` library call is 5–10× slower than manual multiplication. This executes **6 times per RK45 step** (one per `Mass(r_i)` evaluation).
- **Fix:** Replace with `dx*dx*dx` and `dx*dx`.
- **Impact:** ~5–10% faster per RK45 step. Zero accuracy loss.
- **Location:** `vendor/damascus/external/obscura/external/libphysica/src/Numerics.cpp` line ~237.

---

## 4. Replace `pow(x, 0.25)` with `sqrt(sqrt(x))`

- **What:** `RK45_Next_Step_Size()` computes `pow(tolerances[i] / errors[i], 0.25)`. `sqrt()` is a single hardware instruction on Apple Silicon; `pow()` is a full library call.
- **Fix:** Use `sqrt(sqrt(tolerances[i] / errors[i]))`.
- **Impact:** Small but free. Only invoked when a step is rejected.
- **Location:** `vendor/damascus/src/Simulation_Trajectory.cpp` line ~99.

---

## 5. Reduce Snapshot Callback Frequency

- **What:** `Publish_Snapshot_Progress()` is called **after every single RK45 step**. Even when no callback is registered, this incurs a `std::function` null-check and function-call overhead.
- **Fix:** Call it only every N steps (e.g. `if ((time_steps % 1000) == 0)`).
- **Impact:** Removes per-step overhead. A few percent for long runs.
- **Location:** `vendor/damascus/src/Simulation_Trajectory.cpp` inside `Propagate_Freely()`.

---

## 6. Fine-Grained 1D Mass Lookup Table

- **What:** `Mass(r)` evaluates a Steffen spline (~22 data points) via `Locate()` + cubic polynomial. All 6 RK stages query closely-spaced radii. A dense uniform grid with linear interpolation replaces the spline search with direct indexing and 2 loads + 3 FMAs.
- **Trade-off:** Loses Steffen’s monotonicity guarantee, but a 10,000-point grid gives negligible error for the smooth AGSS09 mass profile.
- **Impact:** Potentially 2–3× faster per `Mass()` call.
- **Location:** Would add a helper to `Solar_Model.hpp/cpp`.

---

## 7. Switch from Fehlberg to Dormand–Prince (DOPRI5)

- **What:** The current method is RKF45. Dormand–Prince is a different 5(4) pair with a better error estimator and more efficient step-size controller. For smooth orbital problems it typically needs **10–20% fewer function evaluations**.
- **Fix:** Swap the Butcher tableau coefficients. Code structure stays identical.
- **Impact:** 10–20% fewer `Mass()` evaluations. Accuracy maintained.
- **Location:** `vendor/damascus/src/Simulation_Trajectory.cpp`.

---

## 8. Analytical Kepler Propagation Outside the Sun

- **What:** When `r > rSun`, `Mass(r) = mSun` (constant) and the motion is pure Kepler (two-body problem), which has a known analytical solution.
- **Idea:** Convert state → orbital elements, propagate analytically to the next event, convert back.
- **Caveat:** Only helps for trajectories that spend significant time outside the Sun. For capture studies most time is interior.
- **Impact:** Potentially 10–100× speedup for exterior-dominated trajectories. Exact solution, zero numerical error.
- **Location:** Would modify `Propagate_Freely()` to branch when `r > rSun`.

---

## 9. SIMD Across Multiple Trajectories

- **What:** Pack 2 trajectories (Apple M4 NEON: 2 doubles/vector) and run them in lockstep. All trajectories execute the same RK45 arithmetic on different data.
- **Challenge:** Step-size adaptation branches; lanes desynchronize if rejection rates differ. Only viable if rejections are rare (<5%).
- **Impact:** Theoretical 2× (NEON). Significant code restructuring.
- **Location:** Would require redesigning the trajectory loop in `main.cpp`.

---

## Summary

| # | Optimization | Effort | Speedup | Accuracy |
|---|-------------|--------|---------|----------|
| 1 | `-march=native` | Trivial | 5–15% | Same |
| 2 | **RK5 local extrapolation** | Very easy | **20–35% fewer steps** | **Better** |
| 3 | Replace `pow(x,2/3)` with `x*x*x` | Very easy | 5–10% | Same |
| 4 | `sqrt(sqrt(x))` vs `pow(x,0.25)` | Very easy | Small | Same |
| 5 | Reduce snapshot callback freq | Very easy | ~5% | Same |
| 6 | Fine-grained mass lookup table | Medium | 10–20% | ~Same |
| 7 | Dormand–Prince (DOPRI5) | Medium | 10–20% fewer evals | Same |
| 8 | Analytical Kepler (`r > rSun`) | Hard | 10–100× (exterior) | Exact |
| 9 | SIMD across trajectories | Hard | 2× (NEON) | Same |

**Recommended starting point:** Items 1–5 are free or near-free wins that should yield **25–40% combined speedup** with minimal code changes and zero physics risk.
