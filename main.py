import json
import os
import sys
import tkinter as tk
from tkinter import filedialog, messagebox

from PIL import Image, ImageChops, ImageFilter, ImageTk


APP_TITLE = "Limpia Imagenes - Ruido Negro"
PREVIEW_MAX_SIZE = (420, 420)
CONFIG_PATH = "config.json"

DEFAULTS = {
    "strength": 2,
    "threshold": 130,
    "delta": 8,
    "expand": 4,
    "mask_size": 9,
    "fill_size": 6,
    "logo_path": "assets/logo.png",
}


def _normalize_filter_size(value: int, minimum: int = 3) -> int:
    if value < minimum:
        return minimum
    if value % 2 == 0:
        return value + 1
    return value


def denoise_image(
    image: Image.Image,
    strength: int,
    threshold: int,
    delta: int,
    expand: int,
    mask_size: int,
    fill_size: int,
) -> Image.Image:
    """Reduce pepper noise (black dots) using median filtering passes."""
    fill_size = _normalize_filter_size(fill_size)
    mask_size = _normalize_filter_size(mask_size)
    expand = _normalize_filter_size(expand, minimum=1)

    filled = image.filter(ImageFilter.MedianFilter(size=fill_size))
    for _ in range(max(1, strength) - 1):
        filled = filled.filter(ImageFilter.MedianFilter(size=fill_size))

    gray = image.convert("L")
    local_median = gray.filter(ImageFilter.MedianFilter(size=mask_size))
    dark_mask = gray.point(lambda p: 255 if p <= threshold else 0)
    diff = ImageChops.subtract(local_median, gray)
    diff_mask = diff.point(lambda p: 255 if p >= delta else 0)
    mask = ImageChops.multiply(dark_mask, diff_mask)
    if expand > 1:
        mask = mask.filter(ImageFilter.MaxFilter(size=expand))

    cleaned = image.copy()
    cleaned.paste(filled, mask=mask)
    return cleaned


def make_preview(image: Image.Image) -> Image.Image:
    preview = image.copy()
    preview.thumbnail(PREVIEW_MAX_SIZE, Image.LANCZOS)
    return preview


def load_config() -> dict:
    try:
        with open(CONFIG_PATH, "r", encoding="utf-8") as handle:
            data = json.load(handle)
    except (OSError, json.JSONDecodeError):
        data = {}

    config = DEFAULTS.copy()
    config.update({key: data.get(key, DEFAULTS[key]) for key in DEFAULTS})
    return config


def resolve_asset_path(path: str) -> str:
    if not path:
        return path
    base_path = getattr(sys, "_MEIPASS", os.path.abspath("."))
    return os.path.join(base_path, path)


class App:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title(APP_TITLE)

        self.original_image: Image.Image | None = None
        self.processed_image: Image.Image | None = None
        self.original_preview_tk: ImageTk.PhotoImage | None = None
        self.processed_preview_tk: ImageTk.PhotoImage | None = None

        self.config = load_config()
        self.logo_tk: ImageTk.PhotoImage | None = None
        self._build_ui()
        self._load_logo()

    def _build_ui(self) -> None:
        top_bar = tk.Frame(self.root, padx=12, pady=10)
        top_bar.pack(fill=tk.X)

        top_bar.columnconfigure(0, weight=1)
        top_bar.columnconfigure(1, weight=1)
        top_bar.columnconfigure(2, weight=1)

        button_frame = tk.Frame(top_bar)
        button_frame.grid(row=0, column=0, sticky="w")
        load_button = tk.Button(button_frame, text="Cargar imagen", command=self.load_image)
        load_button.pack(side=tk.LEFT)

        strength_frame = tk.Frame(top_bar)
        strength_frame.grid(row=1, column=0, sticky="w", padx=(0, 12))

        tk.Label(strength_frame, text="Intensidad:").pack(side=tk.LEFT)
        self.strength_var = tk.IntVar(value=self.config["strength"])
        self.strength_scale = tk.Scale(
            strength_frame,
            from_=1,
            to=5,
            orient=tk.HORIZONTAL,
            variable=self.strength_var,
            command=self.on_settings_change,
            length=140,
        )
        self.strength_scale.pack(side=tk.LEFT)

        threshold_frame = tk.Frame(top_bar)
        threshold_frame.grid(row=1, column=1, sticky="w", padx=(0, 12))
        tk.Label(threshold_frame, text="Umbral:").pack(side=tk.LEFT)
        self.threshold_var = tk.IntVar(value=self.config["threshold"])
        self.threshold_scale = tk.Scale(
            threshold_frame,
            from_=10,
            to=160,
            orient=tk.HORIZONTAL,
            variable=self.threshold_var,
            command=self.on_settings_change,
            length=140,
        )
        self.threshold_scale.pack(side=tk.LEFT)

        delta_frame = tk.Frame(top_bar)
        delta_frame.grid(row=1, column=2, sticky="w")
        tk.Label(delta_frame, text="Contraste:").pack(side=tk.LEFT)
        self.delta_var = tk.IntVar(value=self.config["delta"])
        self.delta_scale = tk.Scale(
            delta_frame,
            from_=2,
            to=40,
            orient=tk.HORIZONTAL,
            variable=self.delta_var,
            command=self.on_settings_change,
            length=120,
        )
        self.delta_scale.pack(side=tk.LEFT)

        mask_frame = tk.Frame(top_bar)
        mask_frame.grid(row=2, column=0, sticky="w", padx=(0, 12))
        tk.Label(mask_frame, text="Mascara:").pack(side=tk.LEFT)
        self.mask_var = tk.IntVar(value=self.config["mask_size"])
        self.mask_scale = tk.Scale(
            mask_frame,
            from_=3,
            to=15,
            orient=tk.HORIZONTAL,
            variable=self.mask_var,
            command=self.on_settings_change,
            length=120,
        )
        self.mask_scale.pack(side=tk.LEFT)

        fill_frame = tk.Frame(top_bar)
        fill_frame.grid(row=2, column=1, sticky="w", padx=(0, 12))
        tk.Label(fill_frame, text="Relleno:").pack(side=tk.LEFT)
        self.fill_var = tk.IntVar(value=self.config["fill_size"])
        self.fill_scale = tk.Scale(
            fill_frame,
            from_=3,
            to=11,
            orient=tk.HORIZONTAL,
            variable=self.fill_var,
            command=self.on_settings_change,
            length=120,
        )
        self.fill_scale.pack(side=tk.LEFT)

        expand_frame = tk.Frame(top_bar)
        expand_frame.grid(row=2, column=2, sticky="w")
        tk.Label(expand_frame, text="Expansion:").pack(side=tk.LEFT)
        self.expand_var = tk.IntVar(value=self.config["expand"])
        self.expand_scale = tk.Scale(
            expand_frame,
            from_=1,
            to=7,
            orient=tk.HORIZONTAL,
            variable=self.expand_var,
            command=self.on_settings_change,
            length=120,
        )
        self.expand_scale.pack(side=tk.LEFT)

        self.save_button = tk.Button(
            button_frame, text="Descargar imagen corregida", command=self.save_image, state=tk.DISABLED
        )
        self.save_button.pack(side=tk.LEFT, padx=10)

        self.status_label = tk.Label(top_bar, text="Sin imagen cargada")
        self.status_label.grid(row=0, column=1, columnspan=2, sticky="w", padx=10)

        self.logo_label = tk.Label(top_bar)
        self.logo_label.grid(row=0, column=2, sticky="e")

        panels = tk.Frame(self.root, padx=12, pady=12)
        panels.pack(fill=tk.BOTH, expand=True)

        original_frame = tk.LabelFrame(panels, text="Original", padx=10, pady=10)
        original_frame.pack(side=tk.LEFT, expand=True, fill=tk.BOTH, padx=(0, 10))

        self.original_label = tk.Label(original_frame, text="Carga una imagen")
        self.original_label.pack(expand=True)

        processed_frame = tk.LabelFrame(panels, text="Corregida", padx=10, pady=10)
        processed_frame.pack(side=tk.LEFT, expand=True, fill=tk.BOTH)

        self.processed_label = tk.Label(processed_frame, text="Vista previa")
        self.processed_label.pack(expand=True)

    def load_image(self) -> None:
        file_path = filedialog.askopenfilename(
            title="Selecciona una imagen",
            filetypes=[("Imagenes", "*.png *.jpg *.jpeg *.bmp *.tiff *.webp")],
        )
        if not file_path:
            return

        try:
            image = Image.open(file_path).convert("RGB")
        except (OSError, ValueError) as exc:
            messagebox.showerror("Error", f"No se pudo abrir la imagen.\n{exc}")
            return

        self.original_image = image
        self.processed_image = denoise_image(
            image,
            self.strength_var.get(),
            self.threshold_var.get(),
            self.delta_var.get(),
            self.expand_var.get(),
            self.mask_var.get(),
            self.fill_var.get(),
        )
        self._update_previews()

        self.save_button.config(state=tk.NORMAL)
        self.status_label.config(text="Imagen cargada y procesada")

    def _update_previews(self) -> None:
        if self.original_image:
            original_preview = make_preview(self.original_image)
            self.original_preview_tk = ImageTk.PhotoImage(original_preview)
            self.original_label.config(image=self.original_preview_tk, text="")

        if self.processed_image:
            processed_preview = make_preview(self.processed_image)
            self.processed_preview_tk = ImageTk.PhotoImage(processed_preview)
            self.processed_label.config(image=self.processed_preview_tk, text="")

    def on_settings_change(self, _value: str) -> None:
        if not self.original_image:
            return

        self.processed_image = denoise_image(
            self.original_image,
            self.strength_var.get(),
            self.threshold_var.get(),
            self.delta_var.get(),
            self.expand_var.get(),
            self.mask_var.get(),
            self.fill_var.get(),
        )
        self._update_previews()
        self.status_label.config(text="Vista previa actualizada")

    def save_image(self) -> None:
        if not self.processed_image:
            messagebox.showwarning("Aviso", "No hay imagen corregida para guardar.")
            return

        file_path = filedialog.asksaveasfilename(
            title="Guardar imagen corregida",
            defaultextension=".png",
            filetypes=[
                ("PNG", "*.png"),
                ("JPEG", "*.jpg *.jpeg"),
                ("BMP", "*.bmp"),
                ("TIFF", "*.tiff"),
                ("WEBP", "*.webp"),
            ],
        )
        if not file_path:
            return

        try:
            self.processed_image.save(file_path)
        except OSError as exc:
            messagebox.showerror("Error", f"No se pudo guardar la imagen.\n{exc}")
            return

        self.status_label.config(text="Imagen corregida guardada")

    def _load_logo(self) -> None:
        logo_path = resolve_asset_path(self.config.get("logo_path", ""))
        if not logo_path:
            return

        try:
            logo = Image.open(logo_path).convert("RGBA")
        except (OSError, ValueError):
            return

        logo.thumbnail((140, 80), Image.LANCZOS)
        self.logo_tk = ImageTk.PhotoImage(logo)
        self.logo_label.config(image=self.logo_tk)
        self.root.iconphoto(True, self.logo_tk)


def main() -> None:
    root = tk.Tk()
    app = App(root)
    root.minsize(900, 520)
    root.mainloop()


if __name__ == "__main__":
    main()
