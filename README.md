# ScratchPathTracerCUDA

![Showcase Image](docs/images/best_render.png)
> *High-density scene (~hundreds of spheres, all three material types) after 1846 accumulated frames.*

A hardware-accelerated Path Tracer written in C++ and CUDA. This project is heavily inspired by the famous *"Ray Tracing in One Weekend"* series, but completely redesigned to run in parallel on CUDA architecture, enabling interactive real-time rendering.

## 🚀 Features & Implementation Progress

Below is the roadmap and development progress of the project:

- ✅ **Basic Ray Tracing on CUDA**
- ✅ **Anti-Aliasing (AA) & Temporal Accumulation (TA)**
- ✅ **Ray-Object Intersection**
- ✅ **Material System**
  - Supports Lambertian (Diffuse), Metal (Specular), and Dielectric (Glass/Refraction).
- ✅ **Physical Camera**
  - Simulated physical lens (Pinhole -> Lens) with Depth of Field (Aperture) effects.
  - Interactive UI controls via mouse and keyboard.
- ✅ **Camera Upgrades**
  - Ray timing simulation for physically accurate **Motion Blur**.
- ✅ **Basic Geometric Primitives**
  - Support for 1D, 2D, and 3D primitives including Spheres, Quads (Walls), and Boxes.
- ⬜ **Bounding Volume Hierarchy (BVH)**
  - GPU-accelerated spatial subdivision for fast intersection tests, enabling real-time rendering of scenes with millions of polygons.
- ⬜ **Mitsuba 3 Scene Loader**
  - Parsing and loading standardized scenes exported from Mitsuba 3.
- ⬜ **Texture Mapping**
  - Support for Image textures, Checker patterns, and procedural Perlin Noise.
- ⬜ **Emissive Materials**
  - Turning geometric primitives into Area Lights.
- ⬜ **Participating Media (Volumes)**
  - Rendering constant density mediums such as fog, smoke, and clouds.
- ⬜ **Advanced Monte Carlo Integration**
  - *In Progress:* Integrating Importance Sampling, complex Probability Density Functions (PDF), and Next Event Estimation (Direct Light Sampling) for rapid noise reduction in complex scenes.

## 🛠 System Requirements

- **OS:** Windows / Linux
- **GPU:** NVIDIA GPU (CUDA compatible)
- **Tools:**
  - [CUDA Toolkit](https://developer.nvidia.com/cuda-toolkit) (Version >= 11.0)
  - CMake (Version >= 3.18)
  - C++ Compiler (MSVC on Windows, GCC/Clang on Linux)

## ⚙️ Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/your-username/ScratchPathTracerCUDA.git
   cd ScratchPathTracerCUDA
   ```

2. **Configure with CMake:**
   ```bash
   cmake -B build
   ```

3. **Build the project:**
   ```bash
   cmake --build build --config Release
   ```

4. **Run:**
   Navigate to the `build/Release` (or `build/Debug`) directory and execute `ScratchPathTracerCUDA.exe`.

## 🎮 Interactive Controls
- **Left Mouse Drag:** Orbit Camera.
- **Right Mouse Drag:** Pan Camera.
- **Mouse Wheel:** Zoom In / Zoom Out.
- **ImGui Control Panel:** 
  - Adjust `FOV` (Field of View).
  - Adjust `Aperture` (Depth of Field intensity).
  - Adjust `Focus Dist` (Focal distance).
  - Monitor Performance (FPS, render time per frame).

## 📸 Gallery

Showcase renders from the project report (Chapter 4 — *Evaluation*). All images were rendered on an NVIDIA GeForce GTX 1650 at 1920×1014.

### Overall Render Quality
A dense scene of several hundred spheres with all three material types, converged over **1846 accumulated frames** via temporal Monte Carlo accumulation.

![Overall render quality](docs/images/best_render.png)

### Material Reproduction
The three implemented materials side by side: **Lambertian** (red, ideal diffuse `f_r = ρ/π`), **Dielectric** (glass, Snell refraction + Schlick Fresnel), and **Metal** (gold, mirror reflection).

![Three materials](docs/images/three_materials.png)

### Depth of Field
Thin-lens camera with a finite aperture. Objects on the focal plane stay sharp while others spread into the circle of confusion — blur strength scales with aperture radius `R = aperture/2`.

| Aperture 0.061, Focus 0.9 | Aperture 0.021, Focus 2.1 |
|:---:|:---:|
| ![DoF a](docs/images/dof_01.png) | ![DoF b](docs/images/dof_02.png) |

### Motion Blur
Finite exposure `[t₀, t₁]`: each ray carries a random timestamp and moving spheres are linearly interpolated `p(t) = p₀ + t·v`, spreading their energy along the trajectory. Static spheres stay sharp.

![Motion blur](docs/images/motion_blur.png)

### Vignetting
Post-process simulation of the optical `cos⁴θ` falloff law — brightness decreases from the center toward the frame corners, drawing the viewer's eye to the subject.

![Vignetting](docs/images/vignette.png)

---
*This project was built for educational and research purposes in Computer Graphics & Parallel Processing.*
