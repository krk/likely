Gabor Wavelet
-------------

    lambda = 128 ; wavelength
    psi    = 0.0 ; phase offset
    gamma  = 1.0 ; aspect ratio
    sigma  = 64 ; standard deviation
    theta  = 0.0 ; orientation

    sigma_x = sigma
    sigma_y = sigma / gamma

    n_stddev = 3
    x_max = (max (max (fabs n_stddev * sigma_x * theta.cos)
                      (fabs n_stddev * sigma_y * gamma * theta.sin))
                 1).ceil
    y_max = (max (max (fabs n_stddev * sigma_x * theta.cos)
                      (fabs n_stddev * sigma_y * gamma * theta.sin))
                 1).ceil

    (gabor f32 f32 f32 f32 f32 f32 f32) =
      (x_max y_max sigma_x sigma_y theta lambda psi) =>
    {
      dx = x.f32 - x_max
      dy = y.f32 - y_max
      xp = dx * theta.cos + dy * theta.sin
      yp = -1 * dx * theta.sin + dy * theta.cos
      (exp -0.5 * ((xp * xp) / (sigma_x * sigma_x) + (yp * yp) / (sigma_y * sigma_y))) * (cos 2 * pi / lambda * xp + psi)
    } : ((columns 2 * x_max + 1) (rows 2 * y_max + 1) (type parallel))

    (gabor x_max y_max sigma_x sigma_y theta lambda psi)
