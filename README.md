# IsCombed

Check whether or not a frame is combed and stores the result in the `_Combed` frame property.

Ported from AviSynth plugin http://bengal.missouri.edu/~kes25c/.


## Parameters

```py
iscombed.IsCombed(vnode clip[, int cthresh=6, int blockx=16, int blocky=16, bint chroma=False, int mi=64, int metric=0])
```

- clip: Clip to process. Only format with integer sample type of 8-16 bit depth and chroma subsampling 1x-4x is supported.

- cthresh: Area combing threshold used for combed frame detection. This essentially controls how "strong" or "visible" combing must be to be detected. Good values are from 6 to 12. If you know your source has a lot of combed frames set this towards the low end (6-7). If you know your source has very few combed frames set this higher (10-12). Going much lower than 5 to 6 or much higher than 12 is not recommended.

- blockx: Specifies the x-axis size of the window used during combed frame detection. This has to do with the size of the area in which `mi` number of pixels are required to be detected as combed for a frame to be declared combed. See the `mi` parameter description for more info. Possible values are any number that is a power of 2 starting at 4 and going to 2048 (e.g. 4, 8, 16, 32, ... 2048).

- blocky: Specifies the y-axis size of the window used during combed frame detection. This has to do with the size of the area in which `mi` number of pixels are required to be detected as combed for a frame to be declared combed. See the `mi` parameter description for more info. Possible values are any number that is a power of 2 starting at 4 and going to 2048 (e.g. 4, 8, 16, 32, ... 2048).

- chroma: Includes chroma combing in the decision about whether a frame is combed. Only use this if you have one of those weird sources where the chroma can be temporally separated from the luma (i.e. the chroma moves but the luma doesn't in a field). Otherwise, it will just help to screw up the decision most of the time.

- mi: The number of required combed pixels inside any of the `blockx` by `blocky` sized blocks on the frame for the frame to be considered combed. While `cthresh` controls how "visible" or "strong" the combing must be, this setting controls how much combing there must be in any localized area (a `blockx` by `blocky` sized window) on the frame. Min setting = 0, max setting = `blockx` x `blocky` (at which point no frames will ever be detected as combed).

- metric: Specifies which spatial combing metric is used to detect combed pixels.

```
Assume 5 neighboring pixels (a,b,c,d,e) positioned vertically.
    a
    b
    c
    d
    e

0:  d1 = c - b;
    d2 = c - d;
    if ((d1 > cthresh && d2 > cthresh) || (d1 < -cthresh && d2 < -cthresh))
    {
       if (abs(a+4*c+e-3*(b+d)) > cthresh*6) it's combed;
    }

1:  val = (b - c) * (d - c);
    if (val > cthresh*cthresh) it's combed;
```

  Metric 0 is what IsCombedTIVTC used previous to v1.2. Metric 1 is the combing metric used in Donald Graft's FieldDeinterlace()/IsCombed() functions in decomb.dll.


## Installation

```
pip install -U vapoursynth-iscombed
```
