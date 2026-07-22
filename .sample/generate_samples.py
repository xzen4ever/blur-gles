import os
import io
import math
from PIL import Image, ImageDraw, ImageFont

def draw_blur_test_pattern(width, height, title):
    # Create RGB canvas with dark sleek background
    img = Image.new("RGB", (width, height), (18, 22, 36))
    draw = ImageDraw.Draw(img)

    # 1. Background: Smooth Color Gradient (Left to Right)
    for x in range(width):
        factor = x / float(width)
        r = int(18 + 30 * factor)
        g = int(22 + 40 * factor)
        b = int(36 + 60 * factor)
        draw.line([(x, 0), (x, height)], fill=(r, g, b))

    # 2. Siemens Star (Star Resolution Target) - Center Left
    cx_star = width // 4
    cy_star = height // 2
    r_star = min(width, height) // 4
    num_spokes = 36
    for i in range(num_spokes):
        angle1 = i * (2 * math.pi / num_spokes)
        angle2 = (i + 0.5) * (2 * math.pi / num_spokes)
        x1 = cx_star + r_star * math.cos(angle1)
        y1 = cy_star + r_star * math.sin(angle1)
        x2 = cx_star + r_star * math.cos(angle2)
        y2 = cy_star + r_star * math.sin(angle2)
        draw.polygon([(cx_star, cy_star), (x1, y1), (x2, y2)], fill=(255, 255, 255))

    # Outer ring for Siemens Star
    draw.ellipse([cx_star - r_star, cy_star - r_star, cx_star + r_star, cy_star + r_star], outline=(255, 200, 50), width=max(2, width // 400))

    # 3. Zone Plate / Concentric Frequency Rings - Center Right
    cx_zone = (width * 3) // 4
    cy_zone = height // 2
    r_zone = min(width, height) // 4
    for r_curr in range(r_zone, 0, -1):
        # Frequency increases quadratically with radius r^2
        val = int(127.5 * (1.0 + math.sin(0.005 * r_curr * r_curr)))
        draw.ellipse([cx_zone - r_curr, cy_zone - r_curr, cx_zone + r_curr, cy_zone + r_curr], fill=(val, val, val))
    draw.ellipse([cx_zone - r_zone, cy_zone - r_zone, cx_zone + r_zone, cy_zone + r_zone], outline=(0, 220, 200), width=max(2, width // 400))

    # 4. USAF 1951 Style Bar Sets (32px, 16px, 8px, 4px, 2px, 1px)
    bar_x = width // 2 - width // 10
    bar_y = height // 5
    bar_height = height // 10
    bar_widths = [32, 16, 8, 4, 2, 1]
    
    current_x = bar_x
    for bw in bar_widths:
        if current_x + bw * 7 > width // 2 + width // 10:
            break
        # Draw 3 vertical parallel bars
        for b_idx in range(3):
            draw.rectangle([current_x, bar_y, current_x + bw - 1, bar_y + bar_height], fill=(255, 255, 255))
            current_x += bw * 2
        current_x += bw * 2

    # Slanted Edge for ISO 12233 MTF Edge Spread Function (ESF/LSF)
    slant_x = width // 2 - width // 16
    slant_y = (height * 3) // 5
    slant_w = width // 8
    slant_h = height // 8
    draw.polygon([
        (slant_x, slant_y),
        (slant_x + slant_w, slant_y - int(slant_w * 0.0873)), # 5-degree slant
        (slant_x + slant_w, slant_y + slant_h),
        (slant_x, slant_y + slant_h)
    ], fill=(255, 255, 255))

    # 5. Header & Technical Overlay Labels
    try:
        font_large = ImageFont.truetype("arial.ttf", max(24, min(width, height) // 18))
        font_small = ImageFont.truetype("arial.ttf", max(14, min(width, height) // 38))
    except IOError:
        font_large = ImageFont.load_default()
        font_small = ImageFont.load_default()

    draw.text((width // 30, height // 30), f"BLUR BENCHMARK TEST CHART — {title}", fill=(255, 255, 255), font=font_large)
    draw.text((width // 30, height // 30 + min(width, height) // 14), f"Resolution: {width} x {height} px | ISO 12233 / Siemens Star / Zone Plate", fill=(180, 210, 255), font=font_small)

    # Labels for Target Components
    draw.text((cx_star - r_star // 2, cy_star + r_star + 10), "Siemens Star (Isotropic)", fill=(255, 200, 50), font=font_small)
    draw.text((cx_zone - r_zone // 2, cy_zone + r_zone + 10), "Zone Plate (Frequency)", fill=(0, 220, 200), font=font_small)
    draw.text((bar_x, bar_y - min(width, height) // 25), "USAF Bar Targets", fill=(255, 255, 255), font=font_small)
    draw.text((slant_x, slant_y + slant_h + 10), "ISO Slanted Edge", fill=(255, 255, 255), font=font_small)

    return img

def save_optimal_format(img, base_path):
    # Encode to JPEG
    jpg_buf = io.BytesIO()
    img.save(jpg_buf, format="JPEG", quality=92, optimize=True)
    jpg_bytes = jpg_buf.getvalue()

    # Encode to PNG
    png_buf = io.BytesIO()
    img.save(png_buf, format="PNG", compress_level=9, optimize=True)
    png_bytes = png_buf.getvalue()

    # Compare file sizes and select the most compact format
    if len(jpg_bytes) <= len(png_bytes):
        chosen_format = "JPEG"
        chosen_bytes = jpg_bytes
        ext = ".jpg"
    else:
        chosen_format = "PNG"
        chosen_bytes = png_bytes
        ext = ".png"

    final_path = base_path + ext
    with open(final_path, "wb") as f:
        f.write(chosen_bytes)

    print(f"Generated: {os.path.basename(final_path)} | Format: {chosen_format} | Size: {len(chosen_bytes) / 1024.0:.1f} KB (JPEG: {len(jpg_bytes)/1024.0:.1f}KB vs PNG: {len(png_bytes)/1024.0:.1f}KB)")
    return final_path

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    assets_dir = os.path.join(project_root, "app", "src", "main", "assets", "samples")

    out_dirs = [script_dir, assets_dir]

    for out_dir in out_dirs:
        os.makedirs(out_dir, exist_ok=True)
        # Clean up legacy .bmp files if any exist
        for fname in os.listdir(out_dir):
            if fname.endswith(".bmp"):
                try:
                    os.remove(os.path.join(out_dir, fname))
                    print(f"Removed legacy BMP: {fname} from {out_dir}")
                except Exception:
                    pass

    print("Generating optimal JPEG/PNG Blur Benchmark Test Charts for project assets & .sample...")
    for out_dir in out_dirs:
        save_optimal_format(draw_blur_test_pattern(1280, 720, "HD 720p"), os.path.join(out_dir, "sample_hd_720p"))
        save_optimal_format(draw_blur_test_pattern(1920, 1080, "Full HD 1080p"), os.path.join(out_dir, "sample_fhd_1080p"))
        save_optimal_format(draw_blur_test_pattern(3840, 2160, "4K UHD 2160p"), os.path.join(out_dir, "sample_4k_2160p"))
