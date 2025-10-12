import numpy as np
from PIL import Image

# ---------------------------
# Core Fourier–series helpers
# ---------------------------

def image_to_array(path_or_array, as_float=True):
    """
    Load an image file or accept a numpy array directly.
    Returns array of shape (H, W) for grayscale or (H, W, 3) for RGB.
    """
    if isinstance(path_or_array, (str, bytes, bytearray)):
        img = Image.open(path_or_array).convert("RGB")
        arr = np.asarray(img)
    else:
        arr = np.asarray(path_or_array)
        if arr.ndim == 2:
            arr = arr[..., None]  # treat as single-channel, will be squeezed later
    if as_float:
        arr = arr.astype(np.float64) / (255.0 if arr.dtype.kind in "ui" else 1.0)
    return arr

def fft2_channels(arr):
    """
    Compute centered 2D FFT (Fourier-series coefficients) per channel.
    Returns coeffs with DC at the center via fftshift.
    """
    if arr.ndim == 2:
        arr = arr[..., None]  # (H, W, 1)
    H, W, C = arr.shape
    coeffs = np.empty((H, W, C), dtype=complex)
    for c in range(C):
        coeffs[..., c] = np.fft.fftshift(np.fft.fft2(arr[..., c]))
    return coeffs

def ifft2_channels(coeffs):
    """
    Inverse of fft2_channels. Expects coeffs with DC at center (shifted).
    Returns real image array in [0, 1] (approx).
    """
    H, W, C = coeffs.shape
    out = np.empty((H, W, C), dtype=np.float64)
    for c in range(C):
        out[..., c] = np.real(np.fft.ifft2(np.fft.ifftshift(coeffs[..., c])))
    if C == 1:
        out = out[..., 0]
    return out

def lowpass_mask(shape, keep_radius_y, keep_radius_x):
    """
    Create a centered rectangular low-pass mask keeping frequencies within
    |ky| <= keep_radius_y and |kx| <= keep_radius_x.
    """
    H, W = shape
    ky = np.fft.fftshift(np.fft.fftfreq(H)) * H  # integer-like frequency indices
    kx = np.fft.fftshift(np.fft.fftfreq(W)) * W
    KY, KX = np.meshgrid(ky, kx, indexing="ij")
    return (np.abs(KY) <= keep_radius_y) & (np.abs(KX) <= keep_radius_x)

def keep_low_frequencies(coeffs, keep_frac=0.1, anisotropic=False):
    """
    Zero all but the lowest spatial frequencies.
    keep_frac: fraction of size to keep (0..1). If anisotropic=False, uses the same
               fraction in both axes. If True, expects a (fy, fx) tuple.
    """
    H, W, C = coeffs.shape
    if isinstance(keep_frac, tuple):
        fy, fx = keep_frac
    else:
        fy = fx = keep_frac
    mask = lowpass_mask((H, W), int((H//2)*fy), int((W//2)*fx))
    filt = np.zeros_like(coeffs)
    for c in range(C):
        filt[..., c] = coeffs[..., c] * mask
    return filt

def keep_topk_magnitude(coeffs, k):
    """
    Keep the K largest-magnitude Fourier coefficients (per whole image, shared across channels).
    Returns a sparse coefficient array of same shape.
    """
    H, W, C = coeffs.shape
    # aggregate magnitude across channels so we keep same positions for all channels
    mag = np.sqrt(np.sum(np.abs(coeffs)**2, axis=2))
    flat_idx = np.argpartition(mag.ravel(), -k)[-k:]  # indices of top-K (unordered)
    mask = np.zeros(H*W, dtype=bool)
    mask[flat_idx] = True
    mask = mask.reshape(H, W)
    out = np.zeros_like(coeffs)
    out[mask, :] = coeffs[mask, :]
    return out

def fs_to_dict(coeffs):
    """
    Convert coefficients array (with DC at center) into a dict mapping
    (ky, kx) -> complex coefficient for grayscale, or -> complex[3] for RGB.
    Frequencies are integer indices consistent with DFT periodic basis.
    """
    H, W, C = coeffs.shape
    ky = np.fft.fftshift(np.fft.fftfreq(H)) * H
    kx = np.fft.fftshift(np.fft.fftfreq(W)) * W
    KY, KX = np.meshgrid(ky, kx, indexing="ij")
    d = {}
    for i in range(H):
        for j in range(W):
            key = (int(KY[i, j]), int(KX[i, j]))
            val = coeffs[i, j] if C > 1 else coeffs[i, j, 0]
            if np.any(val != 0):
                d[key] = val
    return d

# ---------------------------
# Example usage
# ---------------------------

if __name__ == "__main__":
    # Load as float in [0,1]
    img = image_to_array("test3/download.png")  # or pass a numpy array directly

    # 1) Forward FS (DFT coefficients), centered
    coeffs = fft2_channels(img)

    # 2a) Keep only low frequencies (e.g., 10% in each dimension)
    coeffs_low = keep_low_frequencies(coeffs, keep_frac=0.10)
    recon_low = ifft2_channels(coeffs_low)

    # 2b) Or keep the K largest coefficients (sparse FS)
    K = 5000
    coeffs_topk = keep_topk_magnitude(coeffs, K)
    recon_topk = ifft2_channels(coeffs_topk)

    # 3) Optional: export as a dictionary of Fourier-series coefficients
    fs_dict = fs_to_dict(coeffs_topk)  # {(ky, kx): complex or complex[3], ...}

    # 4) Save results (will clip to [0,1] and convert to 8-bit)
    def save_img(path, arr):
        a = np.clip(arr, 0.0, 1.0)
        if a.ndim == 2:
            a = (a * 255).astype(np.uint8)
            Image.fromarray(a, mode="L").save(path)
        else:
            a = (a * 255).astype(np.uint8)
            Image.fromarray(a).save(path)

    save_img("recon_low.png", recon_low)
    save_img("recon_topk.png", recon_topk)

    # 5) (Optional) Report compression ratio
    H, W = coeffs.shape[:2]
    kept_low = np.count_nonzero(np.abs(coeffs_low).sum(axis=2))
    kept_topk = np.count_nonzero(np.abs(coeffs_topk).sum(axis=2))
    print(f"Low-pass kept {kept_low}/{H*W} frequency locations")
    print(f"Top-K kept   {kept_topk}/{H*W} frequency locations")
