Average Face
------------
Compute the average face from a set of aligned faces [1].

    avg = m =>
    {
      j = 0
      (j = j + m) $ t m.frames
      j / m.frames
    } : ((frames 1) (type parallel))

    "../data/lfwa.tar.gz".read.avg

Command Line

```bash
$ likely ../library/average_face.ll -show
$ likely ../library/average_face.ll -record average_face.png
$ dream ../library/average_face.ll
```

[1] http://www.openu.ac.il/home/hassner/data/lfwa/

[Previous](?show=mandelbrot_set)
