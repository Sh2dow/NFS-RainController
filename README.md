# NFS.WeatherMod

### Features

The following feature classes are declared:

- `ForcePrecipitation` – standalone implementation of the
  precipitation effects, inspired by NFS Carbon


<pre> 
✅ Completed Features

- 🔄 Config loading	✅	RainConfigController::Load() is called on enable()
- 🌧️ 3D Rain Drop Angle	✅	Drop3D::angle added and used with rotated quad rendering
- 🌬️ Perlin Wind in 2D	✅	Per-drop noiseSeed, time-based, drop.y-dependent
- 🎯 Drop respawn randomness	✅	Wider spawn range (±200 X, 100–1100 Z)
- 💧 Splatters	✅	3D splatters implemented, fade over time, randomized size
- 🎛️ Alpha & Intensity scaling	✅	All rain effects respond to config’s rainIntensity
- 🌀 Perlin noise setup	✅	siv::PerlinNoise noise{ std::random_device{} }; is present and used
- 🔄 2D drop logic	✅	Uses wind + speed + wraparound, randomized properties
- 🎨 Textured 3D rain	✅	Dynamic texture or fallback procedural texture created
- 🌪️ 3D rain Perlin wind ✅ use noise.noise3D(...) like the 2D version for realism
- 💫 Splatter angles / rotations ✅ 3D splatters are rendered with unrotated quads — consider randomizing angle & rotating like drops
</pre> 

<pre> Next to do:

- 🌁 Rain occlusion under bridges	📝 Requires depth sampling or zone detection — still not implemented
- 🪟 Wet screen distortion overlay 📝 Not yet rendered — would require a fullscreen pass using a distortion texture or shader
</pre> 
