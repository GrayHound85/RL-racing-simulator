# RL Racing Simulator — Project Plan & Roadmap

_Nov 12, 2026 · @Oliver_

## Vision and goals

The goal is a racing simulator where you design tracks, train AI drivers with your own reinforcement learning (RL), and race against them. It is written in modern C++, with the physics and machine learning built from scratch.

### Two goals, in this order

1. **Learning goal (primary):** become strong at modern C++. That means ownership, value types, templates, performance, build systems, testing and architecture. When a choice teaches more C++, prefer it.
2. **Product goal (secondary):** a working, playable simulator. It keeps the project motivating and gives every module a real user.

When the two conflict, pick the option you will understand more deeply, not the one that ships faster.

### What "done" means at each level

- **Minimum success:** a neural network you wrote learns, with PPO you wrote, to lap a 2D track. You can watch it drive.
- **Full success:** a 3D view, a track editor, a training dashboard, saved models, and racing against your own AI.
- **Stretch:** multithreaded training, GPU compute, realistic tyre physics, AI that beats you.

Minimum success is the target to protect. Everything after it is a bonus.

### How to use this doc

- Treat it as the source of truth for architecture and order of work.
- When you change a decision, edit the doc and note why in the Decision log.
- Before starting a milestone, reread its section and the matching learning topics.

## Guiding principles

These ten rules settle most arguments before they start. If a design breaks one, it needs a written reason.

1. **Vertical slices over layers.** Always keep something working end to end, however ugly. Improve one part at a time. Never spend months on a layer that nothing uses yet.
2. **Build only what the next consumer needs.** No general tensor library, no feature "for later". A function exists because something calls it.
3. **Custom where it teaches, libraries where it doesn't.** Write your own maths, neural networks, PPO and car physics. Use libraries for windows, OpenGL loading, UI widgets, tests and file parsing.
4. **Headless first.** Simulation and training never need a window. The app is just one user of the libraries.
5. **Determinism.** Same seed and same inputs give exactly the same result. Fixed timestep, seeded random numbers, no hidden global state.
6. **Dependencies point one way.** Lower layers never include higher ones. PPO never knows about cars; the simulation never knows about neural networks.
7. **Test the maths, watch the behaviour.** Maths and physics get automated tests with known answers. Learning gets graphs and visual checks.
8. **Measure before optimising.** No threads, SIMD or GPU until a profiler shows where time goes.
9. **Simple first, then realistic.** Kinematic car before tyre model, REINFORCE before PPO, CPU before GPU.
10. **Understand every line.** If you copy a pattern, you should be able to explain why it is there.

### Allowed dependencies

| Library | Purpose | When it arrives |
|---|---|---|
| C++ standard library | Everything general | Now |
| Catch2 or doctest | Unit tests | M0 |
| raylib, or SDL2 | 2D debug viewer | M1 |
| GLFW + glad | Window and OpenGL loading for the 3D renderer | M7 |
| Dear ImGui | Debug UI, dashboard, editor panels | M1 or M5 |
| nlohmann/json (or similar) | Track, config and metadata files | M5 |
| A small maths header such as glm | Only for 3D rendering matrices, optional | M7 |

Not allowed: ML libraries (PyTorch, Eigen for ML, etc.), physics engines (Box2D, Bullet), game engines. These would remove the learning.

Add libraries with CMake FetchContent or a package manager (vcpkg or Conan), never by copying files in by hand.

## Architecture

The project is a set of small static libraries with strict one-way dependencies, plus thin executables on top. Each library has its own namespace, folder, CMake target and test executable.

### Modules

| Module (CMake target) | Namespace | Responsibility | Depends on |
|---|---|---|---|
| `racing_core` | `racing::core` | Vec2, small matrices, random numbers, maths helpers, logging | nothing |
| `racing_simulation` | `racing::simulation` | Car physics, tracks, collisions, fixed-timestep stepping | core |
| `racing_ml` | `racing::ml` | Matrices for networks, layers, activations, loss, backprop, optimisers (Adam) | core |
| `racing_rl` | `racing::rl` | Environment interface, rollout buffer, GAE, policy distributions, PPO | ml, core |
| `racing_envs` | `racing::envs` | Adapters that turn things into RL environments: CartPole, point-mass, the car | rl, simulation |
| `racing_training` | `racing::training` | Training jobs, parallel environments, checkpoints, metrics | rl, envs |
| `racing_render` | `racing::render` | 2D debug drawing, later the 3D OpenGL renderer | simulation (read-only) |
| `racing_app` and tools | — | Executables: game, headless trainer, track editor | everything |

### The dependency rules

- **Core** includes only the standard library.
- **Simulation** and **ML** never include each other. They are siblings.
- **RL** only sees the abstract environment interface. It must train CartPole with zero changes.
- **Envs** is the only place where "car" meets "RL". Observation, action and reward code lives here.
- **Render** reads simulation state through `const` references. It never changes it.
- **The headless trainer** links everything except render and app.

Use PRIVATE links by default. Use PUBLIC only when a library's public headers include another library's headers.

### The environment interface

This is the most important boundary in the project. Every environment (CartPole, the car, future ones) offers the same three things:

- `reset(seed)` returns the first observation.
- `step(action)` returns the next observation, a reward, and whether the episode ended (finished, crashed or timed out).
- A description of sizes: observation length, and the action type and range.

Observations and actions are plain arrays of numbers (for example `std::vector<float>` or `std::span<const float>`). RL code never sees a `CarState`.

Keep "terminated" (crashed, finished) separate from "truncated" (time limit). PPO's value estimate treats them differently, and mixing them up is a classic bug.

### Folder layout

```text
CMakeLists.txt
cmake/                  # warnings, sanitizers, helper functions
include/racing/<module>/...
src/<module>/...
tests/<module>/...
apps/
  sim_demo/             # current demo
  trainer/              # headless training CLI
  game/                 # the full application
third_party/            # only if FetchContent is not used
docs/
```

Each module folder has its own `CMakeLists.txt`. The top-level file only sets project options and adds subdirectories.

## Key design decisions

These are decided now so later code doesn't have to guess. Each one can change, but only through the Decision log.

### Time and determinism

- **Fixed physics timestep: 120 Hz** (as in the current demo). The app uses an accumulator loop: render as fast as you like, step physics in fixed chunks.
- **Rendering interpolates** between the last two physics states. It never steps physics itself.
- **All randomness goes through one seeded generator type** in core (e.g. a wrapper over `std::mt19937_64`). No `rand()`, no `std::random_device` outside seeding.
- **Same seed and inputs give identical results.** A test checks this.
- **Open question:** keep `delta_time` as a `step()` argument, or move it into `SimulationConfig`? Moving it makes the fixed timestep impossible to break.

### Physics: 2D, rendered in 3D

- Car physics stays **2D top-down** for the whole project unless a later decision says otherwise.
- The 3D renderer draws the 2D world with height added only for looks.
- Physics grows in stages: kinematic bicycle (now), then a sideways-grip limit, then a dynamic bicycle model with a simple tyre model (slip angles, maybe Pacejka later).
- Units are SI everywhere: metres, seconds, radians, kilograms. Write the unit in a comment or the name.

### Track representation

- A track is a **closed spline centreline** (Catmull-Rom is a good start) plus a **width**, which can vary along the track.
- From that you get: progress along the track (distance travelled along the spline), signed distance from the centreline, edges for drawing, and on/off-track checks.
- Precompute a dense list of sample points along the spline for fast lookups.
- Store tracks as JSON with a format version number.

### RL environment for the car

These are first choices. Expect to change them during M4.

| Part | First choice | Why |
|---|---|---|
| Observation | About 9 raycast distances to the track edge, speed, heading error from the track direction, sideways offset | Doesn't depend on the track, so the AI can drive tracks it has never seen |
| Action | 2 continuous values: steering in [-1, 1], throttle or brake in [-1, 1] | Fits a Gaussian policy; fewer outputs is easier |
| Reward | Progress along the centreline each step, a small time penalty, a large penalty for leaving the track | Progress is dense and hard to game |
| Episode end | Off track (terminated), lap done (terminated), time limit (truncated) | Keep terminated and truncated separate |
| Normalisation | Scale observations to roughly [-1, 1]; keep running mean and std later | Unscaled inputs are a top reason networks don't learn |

### Numbers and types

- **Simulation uses `double`.** It's safe and the cost is low.
- **ML uses `float`.** Networks don't need double, and float is twice as fast for SIMD and GPU later.
- Convert at the environment boundary (in `racing_envs`).
- Use `std::size_t` for sizes and indices. Turn on sign-conversion warnings to catch mixing.

### Error handling

- **Exceptions** for things that can go wrong at runtime even in correct code: a bad config file, a missing track, an invalid user-made track.
- **Asserts** for programmer bugs: NaN in physics, mismatched matrix sizes, an index out of range. They are checked in Debug builds and free in Release builds.
- Validate at the edges (loading, user input) and trust data inside.
- The current `step()` validation is fine for now. Revisit it when training performance matters.

### Ownership and style

- Value types by default (`Vec2`, `CarState`, configs). Avoid `new` and `delete`.
- `std::unique_ptr` for single ownership of polymorphic things (e.g. an `Environment`). Use `shared_ptr` only with a written reason.
- Pass small types by value, big ones by `const&`. Use `std::span` for views of arrays.
- `const` everything that can be. `[[nodiscard]]` on functions whose result matters.
- Interfaces with `virtual` only at real boundaries (Environment). Templates and concrete types elsewhere.

## Roadmap

There are ten milestones. M4, the first lap driven by your own AI, is the one that matters most. Every milestone ends with something you can run and show. Don't start the next one until the current one's done-criteria pass.

### M0 — Foundation (in progress)

**Goal:** a clean, tested, warning-free build that is easy to extend.

- [ ] Fix the broken `target_link_libraries` and commit
- [ ] Split `racing_core` and `racing_simulation` into separate libraries
- [ ] Shared `racing_warnings` INTERFACE target; add `-Wconversion -Wsign-conversion -Wshadow`
- [ ] Debug builds with AddressSanitizer and UndefinedBehaviorSanitizer
- [ ] Test framework plus CTest; first physics tests (rest, no negative speed, top speed 50 m/s, circle radius, determinism)
- [ ] `Vec2` value type in core, test-driven; `CarState` uses it; remove `distance()`
- [ ] Sideways-acceleration limit, with a test
- [ ] `.clang-format`, `.gitignore`, a short README

**Done when:** `cmake --build` and `ctest` pass with zero warnings in Debug (with sanitizers) and Release.

**C++ you practise:** CMake targets and PUBLIC/PRIVATE/INTERFACE, operator overloading, `constexpr`, value semantics, floating-point comparison in tests.

**Pitfall:** comparing doubles with `==` in tests. Use an approximate match with a tolerance.

### M1 — Drive it yourself (2D)

**Goal:** a window where you drive the car with the keyboard around an oval.

- A `sim_viewer` app using raylib (simplest) or SDL2
- A fixed-timestep accumulator loop with interpolation
- First `Track` type: a spline centreline plus width; an oval and a figure-of-eight
- Track queries: progress, sideways offset, on-track test, raycasts to the edges
- Draw the car, track edges and rays; show speed and progress as text

**Done when:** you can drive laps, leaving the track is detected, and the rays look right.

**C++ you practise:** separating app code from library code, `std::span`, algorithms (`std::lower_bound` for spline lookups), RAII wrappers for library resources.

**Pitfall:** putting game logic in the viewer. The viewer only reads state and sends inputs.

### M2 — Neural network from scratch

**Goal:** a small multilayer perceptron (MLP) that learns simple functions, with verified gradients.

- A `Matrix` class (float, row-major, contiguous `std::vector<float>` storage)
- Matrix multiply, add, transpose, element-wise operations
- A `Linear` layer, tanh and ReLU activations, mean-squared-error loss
- Hand-written backpropagation for each layer
- Optimisers: SGD, then Adam
- **Gradient check** with finite differences for every layer
- Train it on XOR, then on fitting sin(x)

**Done when:** gradient checks agree within about 1e-4 relative error, XOR is solved, and the sine fit looks right when plotted.

**C++ you practise:** class design for numeric types, move semantics (big matrices), cache-friendly loops, possibly templates.

**Pitfall:** building automatic differentiation or general N-dimensional tensors. You don't need them.

### M3 — RL on toy problems

**Goal:** a PPO you wrote solves CartPole.

- The `Environment` interface (see Architecture)
- Write CartPole yourself, from the classic equations
- Policy distributions: categorical (discrete actions), diagonal Gaussian (continuous); log-probability and entropy, with gradients
- **Climb the ladder:** REINFORCE, then REINFORCE with a baseline, then actor-critic with GAE, then PPO with the clipped objective
- Rollout buffer, advantage normalisation, minibatches, several epochs per batch
- Log mean episode return, policy loss, value loss, entropy, KL divergence and clip fraction to CSV, then plot them
- A continuous-control test env (e.g. push a point mass to a target)

**Done when:** CartPole reaches the 500-step maximum reliably across 3 or more seeds, and the point-mass task is solved.

**C++ you practise:** abstract interfaces, `std::unique_ptr`, separating algorithm from data, reproducible random numbers.

**Pitfall:** going straight to PPO. If it doesn't learn, you won't know which of 20 parts is wrong.

### M4 — First AI lap (the key milestone)

**Goal:** PPO learns to lap the oval.

- A `CarEnv` in `racing_envs`: observations, actions and reward as in Key design decisions
- Train headless, then load the trained weights into the M1 viewer to watch
- Tune: reward scale, observation scaling, network size (e.g. 2 hidden layers of 64), learning rate
- Then the figure-of-eight, and a track it has never seen

**Done when:** the AI completes laps of the oval without leaving the track, and you can watch it.

**Pitfall:** reward hacking. The AI will find loopholes (e.g. driving backwards and forwards over the start line). Watch it drive, don't just read the numbers.

### M5 — Training system

**Goal:** training that is fast, repeatable and doesn't lose work.

- A `trainer` command-line app: config file in, checkpoints and logs out
- Run N environments in one process (vectorised envs), first one after another, then on threads
- Checkpoints: weights, optimiser state, random generator state, step count, config, format version
- Resume from a checkpoint and get the same results as an uninterrupted run
- Training in a background thread inside the viewer, with a live reward graph (ImGui)

**Done when:** a training run can be stopped and resumed, 8 environments run in parallel, and the viewer shows live progress.

**C++ you practise:** `std::thread` / `std::jthread`, `std::atomic`, mutexes, a thread-safe queue, binary file input and output, `std::filesystem`.

**Pitfall:** data races between the trainer and the viewer. Hand over copies or snapshots, not shared mutable state.

### M6 — Better physics and tracks

**Goal:** driving that feels like a car, on tracks you design.

- Dynamic bicycle model: mass, yaw inertia, tyre slip angles, a simple tyre force curve
- Weight transfer (optional), quadratic aero drag, rolling resistance
- Car-to-wall collision response
- A 2D track editor: place and drag spline points, set width, save and load JSON
- Retrain the AI on the new physics

**Done when:** the car can slide and spin, you can build and save a track, and the AI can learn to drive it.

**Pitfall:** stiff tyre equations at low speed making the physics unstable. Study how others handle it.

### M7 — 3D renderer

**Goal:** the same simulation, drawn in 3D.

- GLFW window, glad, OpenGL 3.3 core profile
- RAII wrappers for shaders, buffers, vertex arrays and textures
- Generate a track mesh from the spline; a simple car model (boxes are fine)
- Chase camera, basic lighting, a ground plane

**Done when:** you can watch an AI lap in 3D from a chase camera, with physics untouched.

**C++ you practise:** RAII for C APIs, move-only types, deleted copy constructors.

### M8 — Full application

**Goal:** something a friend could use.

- Menus: pick a track, pick a model, race, train
- Model browser (saved checkpoints with metadata)
- Training dashboard
- Player against AI: several cars, lap timing, positions
- Car-to-car collisions

**Done when:** you can make a track, train an AI on it, and race it, all inside the app.

### M9 — Optimisation

**Goal:** faster training, guided by measurement.

- Profile with perf, or Tracy, or a sampling profiler
- Typical wins: matrix multiply loop order and blocking, fewer allocations, structure-of-arrays for many cars, threading the rollouts
- SIMD (compiler auto-vectorisation first, then intrinsics if needed)
- GPU compute (OpenGL compute shaders or CUDA) only if profiling says the network is the bottleneck

**Done when:** steps per second are measured before and after every change, and at least doubled overall.

## Learning and research guide

Learn each topic just before the milestone that needs it, not all up front. The "When" column says which milestone. Resources are well-known names to search for. Check each is still current before you buy anything.

### Modern C++

| Topic | What to understand | When |
|---|---|---|
| Value semantics and RAII | Why objects clean up after themselves; rule of zero, three and five | M0 |
| Operator overloading | Which operators to provide for `Vec2`, member or free function, `constexpr` | M0 |
| `const` correctness and references | `const&` versus by-value; when a copy is cheaper | M0 |
| Header and source split | What goes in headers, inline functions, include guards or `#pragma once`, forward declarations | M0 |
| Move semantics | rvalue references, `std::move`, why moving a big matrix is cheap | M2 |
| Templates basics | Function and class templates; when to template a `Matrix<T>` (maybe not yet) | M2 |
| Concepts (C++20) | Constraining templates with readable errors | M2 or later |
| Smart pointers | `unique_ptr` ownership, why `shared_ptr` is rarely needed | M3 |
| Virtual functions and interfaces | Abstract base classes, virtual destructors, cost of virtual calls | M3 |
| `std::span`, `std::array`, `std::vector` | Owning versus viewing; contiguous memory | M1 to M2 |
| Algorithms and ranges | `<algorithm>`, `std::ranges`, writing loops less | M1 onward |
| Undefined behaviour | Signed overflow, out-of-bounds, uninitialised reads; why sanitizers matter | M0, ongoing |
| Concurrency | `std::thread`, `std::jthread`, `std::mutex`, `std::atomic`, condition variables, data races | M5 |
| Data-oriented design | Cache lines, structure-of-arrays, why memory layout beats clever code | M9 |

**Resources:** learncpp.com (best free start), cppreference.com (reference), _A Tour of C++_ (Stroustrup), _Effective Modern C++_ (Meyers), the C++ Core Guidelines, _C++ Concurrency in Action_ (Williams), CppCon "Back to Basics" talks on YouTube.

### Build, tooling and workflow

| Topic | What to understand | When |
|---|---|---|
| Modern CMake | Targets not variables; PUBLIC, PRIVATE and INTERFACE; `target_*` commands | M0 |
| CMake presets | `CMakePresets.json` for Debug, Release and sanitizer builds | M0 |
| FetchContent | Pulling in Catch2, raylib and others reproducibly | M0 to M1 |
| Unit testing | Arrange-act-assert, approximate float comparisons, test one thing per test | M0 |
| Sanitizers | ASan, UBSan, later TSan (threads) | M0, M5 |
| Debugger | Breakpoints, stepping, watching variables (gdb, lldb or your IDE) | M0 |
| Git habits | Small commits, branches per feature, meaningful messages | M0 |
| clang-format and clang-tidy | Consistent style; automatic bug-pattern checks | M0 to M1 |
| Profiling | perf, Tracy or your IDE's profiler; reading flame graphs | M9 |

**Resources:** _Professional CMake_ (Craig Scott), "An Introduction to Modern CMake" (free online), the "Effective Modern CMake" gist, Catch2 tutorial docs.

### Maths

| Topic | What to understand | When |
|---|---|---|
| 2D vectors and trigonometry | Dot product, length, normalising, rotation, angles with `atan2` | M0 |
| Numerical integration | Euler, semi-implicit Euler, why fixed timesteps matter | M0 |
| Floating point | Rounding error, epsilon comparisons, NaN and infinity | M0 |
| Splines | Catmull-Rom curves, arc length, nearest point on a curve | M1 |
| Ray and segment intersection | For raycast sensors | M1 |
| Linear algebra | Matrix multiply, transpose, shapes; what a layer does geometrically | M2 |
| Calculus | Derivatives, partial derivatives, the chain rule; this is all backprop is | M2 |
| Probability | Expected value, variance, Gaussian distribution, log-probabilities | M3 |
| 3D maths | 4x4 transforms, view and projection matrices, homogeneous coordinates | M7 |

**Resources:** 3Blue1Brown _Essence of Linear Algebra_ and _Essence of Calculus_ (YouTube), Khan Academy for gaps, _Mathematics for Machine Learning_ (Deisenroth, free PDF).

### Car physics

| Topic | What to understand | When |
|---|---|---|
| Kinematic bicycle model | What you have now and its limits (no slip) | M0 |
| Fixed timestep loops | Accumulator, interpolation, "spiral of death" | M1 |
| Dynamic bicycle model | Forces, mass, yaw inertia, slip angle | M6 |
| Tyre models | Linear tyre, then a simplified Pacejka "magic formula" | M6 |
| Drag and rolling resistance | Quadratic air drag, constant rolling force | M6 |
| Collision response | Detecting and resolving car-to-wall contacts, impulses | M6 |

**Resources:** Glenn Fiedler "Fix Your Timestep!" (essential), Marco Monster "Car Physics for Games", Brian Beckman "The Physics of Racing" series, _Game Programming Patterns_ (Nystrom, free online, see the Game Loop chapter).

### Machine learning

| Topic | What to understand | When |
|---|---|---|
| Neurons, layers, MLPs | What a forward pass computes | M2 |
| Activations | tanh, ReLU; why tanh is common in small RL networks | M2 |
| Loss functions | Mean squared error; log-likelihood | M2 |
| Backpropagation | The chain rule applied layer by layer; caching forward values | M2 |
| Gradient checking | Finite differences to test your gradients | M2 |
| Optimisers | SGD, momentum, Adam | M2 |
| Weight initialisation | Xavier/Glorot, He, orthogonal (used in PPO) | M2 |
| Overfitting and generalisation | Why a model works on one track and not another | M4 |

**Resources:** Andrej Karpathy _Neural Networks: Zero to Hero_ (especially the micrograd video), 3Blue1Brown _Neural Networks_ series, Michael Nielsen _Neural Networks and Deep Learning_ (free online).

### Reinforcement learning

| Topic | What to understand | When |
|---|---|---|
| The RL loop | State, action, reward, episode, return, discount factor | M3 |
| Policy gradients | REINFORCE and why it is noisy | M3 |
| Baselines and value functions | Reducing variance with a critic | M3 |
| Advantage and GAE | Generalised Advantage Estimation and its lambda | M3 |
| PPO | Clipped objective, epochs, minibatches, entropy bonus, KL monitoring | M3 |
| Continuous actions | Gaussian policy, learned log standard deviation, squashing or clipping | M3 to M4 |
| Reward design | Shaping, sparse versus dense reward, reward hacking | M4 |
| Observation normalisation | Running mean and std | M4 |
| Hyperparameters | Learning rate, batch size, gamma, lambda, clip range; how to tune one at a time | M4 |

**Resources:** _Reinforcement Learning: An Introduction_ (Sutton and Barto, free PDF; chapters 3, 6 and 13), OpenAI _Spinning Up in Deep RL_ (very practical), the PPO paper (Schulman et al., 2017), the GAE paper (Schulman et al., 2015), the blog post "The 37 Implementation Details of PPO" (vital for debugging), CleanRL's single-file `ppo.py` as a reference to read.

### Graphics

| Topic | What to understand | When |
|---|---|---|
| 2D drawing with a simple library | Coordinate systems, world-to-screen transform, camera follow | M1 |
| OpenGL basics | Context, buffers, vertex arrays, shaders, the draw call | M7 |
| Shaders (GLSL) | Vertex and fragment shaders, uniforms | M7 |
| Meshes | Generating a track mesh from a spline | M7 |
| Lighting | Simple directional (Phong or Blinn-Phong) lighting | M7 |
| Immediate-mode UI | Dear ImGui concepts and integration | M1 or M5 |

**Resources:** LearnOpenGL.com (the standard), raylib examples, Dear ImGui demo and wiki.

### Suggested study order before M2

1. Linear algebra videos (3Blue1Brown), episodes 1 to 4 and 9
2. Calculus videos, chain rule episodes
3. Karpathy's micrograd video: watch it once, then rebuild the ideas in C++ without looking

## Testing, debugging and validation

Automated tests prove that the maths and physics are right. Graphs and watching the agent prove that learning works. You need both, because RL bugs rarely crash. They just make the agent stop improving.

### Three levels of checking

| Level | What | Examples | Runs |
|---|---|---|---|
| Unit tests | One function or class, known answer | `Vec2` maths, matrix multiply, spline length, car at rest stays at rest | Every build (`ctest`) |
| Property tests | A rule that always holds | Speed never negative; same seed gives same result; probabilities sum to 1 | Every build |
| Learning checks | Does training improve? | CartPole reaches 500; the car laps the oval | By hand or nightly, saved as CSV and plots |

### Physics tests with known answers

- **Rest:** zero input from rest means no movement.
- **Top speed:** full throttle converges to acceleration ÷ drag (50 m/s with the current config).
- **Circle:** constant steering gives a circle of radius wheelbase ÷ tan(steering angle).
- **Straight line:** zero steering keeps heading constant.
- **Grip limit:** sideways acceleration never exceeds the cap.
- **Determinism:** two runs with the same inputs match exactly.
- **Timestep sanity:** halving dt gives nearly the same path. This checks your integration.

### Neural network checks

1. **Gradient check every layer.** Nudge each weight by a small amount (e.g. 1e-3 in double, or about 1e-2 in float). Compare (loss(w + h) − loss(w − h)) ÷ 2h with your backprop gradient. Relative error should be below about 1e-4. Do this check in double precision if float is too noisy.
2. **Overfit one tiny batch.** A network should drive loss near zero on 4 examples. If it can't, something is broken.
3. **XOR and sine.** Known-solvable problems.
4. **Shape asserts.** Assert matrix shapes match in every operation.

### RL debugging ladder

If PPO doesn't learn, go down this list in order:

1. Does the environment work? Run it with random actions, then with a hand-written controller. A simple steering rule should be able to lap the oval.
2. Are rewards and observations scaled sensibly (roughly -1 to 1)?
3. Does a simpler algorithm (REINFORCE or actor-critic) learn on the same env?
4. Are terminated and truncated handled separately in GAE?
5. Are log-probabilities of old actions stored correctly, and not recomputed after the update?
6. Is advantage normalisation applied per batch?
7. Check the logged numbers: KL divergence (should stay small, around 0.01 to 0.05), clip fraction (roughly 0.1 to 0.3), entropy (should fall slowly, not collapse at once), and value loss (should fall).
8. Compare one run against a reference PPO in Python (CleanRL) on CartPole with the same hyperparameters. You're comparing learning curves, not copying code.
9. Try 3 or more seeds. One good seed proves nothing.

### Always log

- Mean and max episode return, episode length
- Policy loss, value loss, entropy, KL divergence, clip fraction
- Learning rate, steps per second
- Write CSV files and plot them with a small Python script, or later a live ImGui graph.

### Visual debugging

- Draw the rays, the nearest point on the centreline, and the progress value in the viewer.
- Record episodes (inputs plus seed) so you can replay any run exactly. Determinism makes this free.
