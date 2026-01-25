import os
import textwrap

from PIL import Image, ImageDraw, ImageFont


WINDOW_SIZE = (700, 450)
TEXT_BOX = (30, 30, 350, 200)
BACKGROUND_COLOR = "#f6f4ef"
TEXT_COLOR = "#c01616"
FONT_SIZE = 20
LEEME_ICON_POS = (500, 90)
LEEME_TEXT = """Es importante que leas este fichero antes de instalar en mac.
"""


def load_font() -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        "/System/Library/Fonts/Supplemental/Helvetica.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
    ]
    for path in candidates:
        if os.path.exists(path):
            try:
                return ImageFont.truetype(path, FONT_SIZE)
            except OSError:
                continue
    return ImageFont.load_default()


def wrap_paragraph(text: str, font: ImageFont.ImageFont, draw: ImageDraw.ImageDraw, max_width: int) -> list[str]:
    words = text.split()
    lines: list[str] = []
    current: list[str] = []
    for word in words:
        trial = " ".join(current + [word])
        if draw.textlength(trial, font=font) <= max_width:
            current.append(word)
        else:
            if current:
                lines.append(" ".join(current))
            current = [word]
    if current:
        lines.append(" ".join(current))
    return lines


def wrap_text(text: str, font: ImageFont.ImageFont, draw: ImageDraw.ImageDraw, max_width: int) -> list[str]:
    lines: list[str] = []
    for raw_line in text.splitlines():
        if not raw_line.strip():
            lines.append("")
            continue
        lines.extend(wrap_paragraph(raw_line, font, draw, max_width))
    return lines


def main() -> None:
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    output_path = os.path.join(base_dir, "assets", "dmg-background.png")

    image = Image.new("RGB", WINDOW_SIZE, BACKGROUND_COLOR)
    draw = ImageDraw.Draw(image)
    font = load_font()

    x0, y0, x1, y1 = TEXT_BOX
    max_width = x1 - x0
    lines = wrap_text(LEEME_TEXT.strip(), font, draw, max_width)
    line_height = font.getbbox("Ag")[3] - font.getbbox("Ag")[1]
    line_height = max(line_height, FONT_SIZE + 2)

    y = y0
    for line in lines:
        if y + line_height > y1:
            break
        draw.text((x0, y), line, font=font, fill=TEXT_COLOR)
        y += line_height

    arrow_start = (x1, y0 + 20)
    arrow_end = (LEEME_ICON_POS[0] - 40, LEEME_ICON_POS[1] + 10)
    draw.line([arrow_start, arrow_end], fill=TEXT_COLOR, width=3)
    draw.polygon(
        [
            (arrow_end[0], arrow_end[1]),
            (arrow_end[0] - 10, arrow_end[1] - 6),
            (arrow_end[0] - 10, arrow_end[1] + 6),
        ],
        fill=TEXT_COLOR,
    )

    image.save(output_path)


if __name__ == "__main__":
    main()
