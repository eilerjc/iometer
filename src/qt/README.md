# Iometer Qt GUI — Speedometer Widget

Cross-platform Qt port of the Iometer "Presentation Meter" (speedometer view)
from the original MFC GUI.

## Files

| File | MFC equivalent | Purpose |
|------|----------------|---------|
| `MeterWidget.h/cpp` | `CMeterCtrl` / `MeterCtrl.cpp` | Speedometer gauge (QPainter) |
| `BigMeterWidget.h/cpp` | `CBigMeter` / `BigMeter.cpp` | Presentation Meter window |
| `main.cpp` | — | Standalone animated demo |
| `CMakeLists.txt` | — | CMake build (Qt5 or Qt6) |

## Build

**Requirements:** Qt 5.15+ or Qt 6.x with the Widgets module, CMake ≥ 3.16.

### Linux / macOS
```sh
cd src/qt
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build build
./build/IometerQtMeterDemo
```

### Windows (MSVC + Qt installer)
```cmd
cd src\qt
cmake -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.7.0\msvc2022_64
cmake --build build --config Release
build\Release\IometerQtMeterDemo.exe
```

## Geometry

The speedometer replicates the original `CMeterCtrl` geometry exactly:

| Constant | Value | Meaning |
|----------|-------|---------|
| `NEEDLE_SWEEP` | 270° | Total arc from min to max |
| `PIVOT_ARC_ANGLE` | 45° | Dead-zone half-angle at bottom |
| `NEEDLE_SENSITIVITY` | 3° | Minimum needle movement before redraw |

The needle sweeps clockwise from lower-left (value = 0) through 12 o'clock
(value = max/2) to lower-right (value = max).  The "notch" at the bottom
shows the scale multiplier (e.g., `x1,000`).

`CalculatePoint(θ, r)` from `MeterCtrl.cpp`:
```
real_angle = θ + 90 + PIVOT_ARC_ANGLE   (= θ + 135°)
x = pivot.x + cos(real_angle) · r
y = pivot.y + sin(real_angle) · r        (y increases downward)
```

## MeterWidget API

```cpp
MeterWidget *m = new MeterWidget(parent);

// Set scale (0 = auto-range)
m->setRange(0, 50000, /*autoRange=*/true);

// Update needle (call periodically from your data loop)
m->setValue(12345.0);

// Watermark (red arc showing observed min/max)
m->showWatermark = true;
m->resetWatermark();
```

Auto-ranging ratchets up by decades when value exceeds the current max,
and down when value drops below max/10 — same behaviour as the MFC original.

## BigMeterWidget API

```cpp
BigMeterWidget *bm = new BigMeterWidget(parent);
bm->setTitle("Test name");
bm->setWorkerResult("Local System", "IOps");
bm->setButtonState(/*canStart=*/false, /*canStop=*/true, /*canStopAll=*/true);
bm->updateDisplay(12345.0, "12,345 IOps");

// Signals
connect(bm, &BigMeterWidget::startRequested, this, &MyClass::onStart);
connect(bm, &BigMeterWidget::stopRequested,  this, &MyClass::onStop);
connect(bm, &BigMeterWidget::nextRequested,  this, &MyClass::onNext);
connect(bm, &BigMeterWidget::resultTypeChanged, this, &MyClass::onMetricChanged);
```

## Integrating with Iometer

Call `updateDisplay()` from wherever `CPageDisplay::UpdateDisplay()` is called
(approximately once per second).  Pass the raw double value (for the needle)
and a formatted string (for the large readout).

The result-type combo box lists the same metrics as `IDR_POPUP_DISPLAY_LIST`
in the MFC resource; map its selection to a `result_to_display` index when
wiring up the full Qt port.
