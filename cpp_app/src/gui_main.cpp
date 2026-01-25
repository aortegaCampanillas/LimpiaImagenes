#include "denoise.h"

#include <filesystem>
#include <optional>

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace {
constexpr int kPreviewMaxSize = 420;

QImage mat_to_qimage(const cv::Mat& mat) {
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888)
        .copy();
}

class MainWindow final : public QMainWindow {
  public:
    MainWindow() {
        setWindowTitle("Limpia Imagenes - Ruido Negro (C++)");

        auto* central = new QWidget(this);
        auto* root_layout = new QVBoxLayout(central);

        auto* top_row = new QHBoxLayout();
        load_button_ = new QPushButton("Cargar imagen");
        save_button_ = new QPushButton("Descargar imagen corregida");
        save_button_->setEnabled(false);
        status_label_ = new QLabel("Sin imagen cargada");

        top_row->addWidget(load_button_);
        top_row->addWidget(save_button_);
        top_row->addWidget(status_label_, 1);
        root_layout->addLayout(top_row);

        load_defaults();
        load_settings();

        auto* sliders_layout = new QHBoxLayout();
        sliders_layout->addLayout(build_slider_column({
            {"Intensidad", &strength_, &strength_slider_, 1, 5},
            {"Mascara", &mask_size_, &mask_slider_, 3, 15},
        }));
        sliders_layout->addLayout(build_slider_column({
            {"Umbral", &threshold_, &threshold_slider_, 10, 160},
            {"Relleno", &fill_size_, &fill_slider_, 3, 11},
        }));
        sliders_layout->addLayout(build_slider_column({
            {"Contraste", &delta_, &delta_slider_, 2, 40},
            {"Expansion", &expand_, &expand_slider_, 1, 7},
        }));
        root_layout->addLayout(sliders_layout);

        auto* panels = new QHBoxLayout();
        original_label_ = build_image_panel("Original");
        processed_label_ = build_image_panel("Corregida");
        panels->addWidget(original_label_->parentWidget());
        panels->addWidget(processed_label_->parentWidget());
        root_layout->addLayout(panels, 1);

        setCentralWidget(central);

        connect(load_button_, &QPushButton::clicked, this, &MainWindow::load_image);
        connect(save_button_, &QPushButton::clicked, this, &MainWindow::save_image);
    }

  private:
    struct SliderConfig {
        const char* label;
        int* value;
        QSlider** slider;
        int min;
        int max;
    };

    QVBoxLayout* build_slider_column(std::initializer_list<SliderConfig> configs) {
        auto* layout = new QVBoxLayout();
        for (const auto& config : configs) {
            auto* label = new QLabel(config.label);
            auto* value_label = new QLabel(QString::number(*config.value));
            auto* slider = new QSlider(Qt::Horizontal);
            slider->setRange(config.min, config.max);
            slider->setValue(*config.value);
            if (config.slider) {
                *config.slider = slider;
            }
            auto* row = new QHBoxLayout();
            row->addWidget(label);
            row->addWidget(slider, 1);
            row->addWidget(value_label);
            layout->addLayout(row);

            int* value_ptr = config.value;
            connect(slider, &QSlider::valueChanged, this, [this, value_ptr, value_label](int v) {
                *value_ptr = v;
                value_label->setText(QString::number(v));
                if (!original_.empty()) {
                    reprocess();
                }
            });
        }
        layout->addStretch(1);
        return layout;
    }

    void load_defaults() {
        Config config;
        std::filesystem::path config_path = std::filesystem::path("..") / "config.json";
        load_config(config_path, config);
        strength_ = config.strength;
        threshold_ = config.threshold;
        delta_ = config.delta;
        expand_ = config.expand;
        mask_size_ = config.mask_size;
        fill_size_ = config.fill_size;
    }

    void load_settings() {
        QSettings settings("LimpiaImagenes", "LimpiaImagenesCpp");
        strength_ = settings.value("strength", strength_).toInt();
        threshold_ = settings.value("threshold", threshold_).toInt();
        delta_ = settings.value("delta", delta_).toInt();
        expand_ = settings.value("expand", expand_).toInt();
        mask_size_ = settings.value("mask_size", mask_size_).toInt();
        fill_size_ = settings.value("fill_size", fill_size_).toInt();
        last_dir_ = settings.value("last_dir", QString()).toString();
    }

    void save_settings() const {
        QSettings settings("LimpiaImagenes", "LimpiaImagenesCpp");
        settings.setValue("strength", strength_);
        settings.setValue("threshold", threshold_);
        settings.setValue("delta", delta_);
        settings.setValue("expand", expand_);
        settings.setValue("mask_size", mask_size_);
        settings.setValue("fill_size", fill_size_);
        settings.setValue("last_dir", last_dir_);
    }

    QLabel* build_image_panel(const char* title) {
        auto* box = new QGroupBox(title);
        auto* layout = new QVBoxLayout(box);
        auto* label = new QLabel("Carga una imagen");
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(300, 300);
        layout->addWidget(label, 1);
        return label;
    }

    void load_image() {
        QString start_dir = last_dir_;
        if (start_dir.isEmpty()) {
            start_dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        }
        QString file = QFileDialog::getOpenFileName(
            this, "Selecciona una imagen", start_dir,
            "Imagenes (*.png *.jpg *.jpeg *.bmp *.tiff *.webp)");
        if (file.isEmpty()) {
            return;
        }
        last_dir_ = QFileInfo(file).absolutePath();
        original_ = cv::imread(file.toStdString(), cv::IMREAD_COLOR);
        if (original_.empty()) {
            QMessageBox::warning(this, "Error", "No se pudo abrir la imagen.");
            return;
        }
        reprocess();
        save_button_->setEnabled(true);
        status_label_->setText("Imagen cargada y procesada");
    }

    void save_image() {
        if (processed_.empty()) {
            QMessageBox::information(this, "Aviso", "No hay imagen corregida para guardar.");
            return;
        }
        QString start_dir = last_dir_;
        if (start_dir.isEmpty()) {
            start_dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        }
        QString file = QFileDialog::getSaveFileName(
            this, "Guardar imagen corregida", start_dir + "/imagen_corregida.png",
            "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp);;TIFF (*.tiff);;WEBP (*.webp)");
        if (file.isEmpty()) {
            return;
        }
        last_dir_ = QFileInfo(file).absolutePath();
        if (!cv::imwrite(file.toStdString(), processed_)) {
            QMessageBox::warning(this, "Error", "No se pudo guardar la imagen.");
            return;
        }
        status_label_->setText("Imagen corregida guardada");
    }

    void reprocess() {
        Config config;
        config.strength = strength_;
        config.threshold = threshold_;
        config.delta = delta_;
        config.expand = expand_;
        config.mask_size = mask_size_;
        config.fill_size = fill_size_;
        processed_ = denoise_image(original_, config);
        update_previews();
        status_label_->setText("Vista previa actualizada");
    }

    void update_previews() {
        update_label(original_label_, original_);
        update_label(processed_label_, processed_);
    }

    void update_label(QLabel* label, const cv::Mat& image) {
        if (image.empty()) {
            label->setText("Vista previa");
            label->setPixmap(QPixmap());
            return;
        }
        QImage preview = mat_to_qimage(image);
        QPixmap pix = QPixmap::fromImage(preview)
                          .scaled(kPreviewMaxSize, kPreviewMaxSize, Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation);
        label->setPixmap(pix);
        label->setText(QString());
    }

    void closeEvent(QCloseEvent* event) override {
        save_settings();
        QMainWindow::closeEvent(event);
    }

    QPushButton* load_button_ = nullptr;
    QPushButton* save_button_ = nullptr;
    QLabel* status_label_ = nullptr;
    QLabel* original_label_ = nullptr;
    QLabel* processed_label_ = nullptr;
    QSlider* strength_slider_ = nullptr;
    QSlider* threshold_slider_ = nullptr;
    QSlider* delta_slider_ = nullptr;
    QSlider* expand_slider_ = nullptr;
    QSlider* mask_slider_ = nullptr;
    QSlider* fill_slider_ = nullptr;
    QString last_dir_;

    int strength_ = 2;
    int threshold_ = 130;
    int delta_ = 8;
    int expand_ = 4;
    int mask_size_ = 9;
    int fill_size_ = 6;

    cv::Mat original_;
    cv::Mat processed_;
};
}  // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(900, 520);
    window.show();
    return app.exec();
}
