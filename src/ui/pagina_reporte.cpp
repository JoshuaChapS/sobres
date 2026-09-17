#include "pagina_reporte.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

#include "../nucleo/exportacion.h"
#include "comunes.h"

namespace sobres {
namespace {

QLabel* parrafo(const QString& texto, const char* color) {
  auto* e = new QLabel(texto);
  e->setWordWrap(true);
  e->setStyleSheet(QStringLiteral("color: %1;").arg(QString::fromLatin1(color)));
  return e;
}

}  // namespace

PaginaReporte::PaginaReporte(Estado* estado, QWidget* padre)
    : QWidget(padre), estado_(estado) {
  const Fecha hoy = Estado::hoy();

  mes_ = new QComboBox;
  for (int m = 1; m <= 12; ++m) {
    mes_->addItem(QString::fromStdString(nombreDeMes(m)), m);
  }
  mes_->setCurrentIndex(hoy.mes - 1);

  anio_ = new QSpinBox;
  anio_->setRange(2000, 2100);
  anio_->setValue(hoy.anio);

  auto* boton = new QPushButton(tr("Generar el Excel de este mes"));
  connect(boton, &QPushButton::clicked, this, &PaginaReporte::generar);

  auto* filaMes = new QHBoxLayout;
  filaMes->addWidget(new QLabel(tr("Mes")));
  filaMes->addWidget(mes_);
  filaMes->addWidget(anio_);
  filaMes->addSpacing(16);
  filaMes->addWidget(boton);
  filaMes->addStretch();

  ultimaCarpeta_ =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  queLleva_ = parrafo(QString(), paleta::kTinta);
  resultado_ = parrafo(QString(), paleta::kTintaSuave);

  auto* explicacion = parrafo(
      tr("El archivo lleva una hoja de resumen con el saldo inicial, las "
         "entradas, los gastos, los traspasos y el saldo final de cada sobre, "
         "más el patrimonio al cierre y el gasto por categoría comparado con "
         "el mes anterior. Después, una hoja por cada sobre con su historial "
         "de movimientos del mes y el saldo corriendo renglón por renglón.\n\n"
         "Los montos van como números, no como texto, así que se pueden sumar "
         "y filtrar dentro de Excel."),
      paleta::kTintaSuave);

  auto* titulo = parrafo(tr("Reporte mensual"), paleta::kTinta);
  QFont fuenteTitulo = titulo->font();
  fuenteTitulo.setPointSize(13);
  fuenteTitulo.setBold(true);
  titulo->setFont(fuenteTitulo);

  auto* raiz = new QVBoxLayout(this);
  raiz->setContentsMargins(24, 20, 24, 24);
  raiz->addWidget(titulo);
  raiz->addSpacing(4);
  raiz->addLayout(filaMes);
  raiz->addSpacing(8);
  raiz->addWidget(queLleva_);
  raiz->addSpacing(8);
  raiz->addWidget(explicacion);
  raiz->addSpacing(8);
  raiz->addWidget(resultado_);
  raiz->addStretch();

  connect(estado_, &Estado::cambio, this, &PaginaReporte::refrescar);
  connect(mes_, &QComboBox::currentIndexChanged, this,
          &PaginaReporte::refrescar);
  connect(anio_, &QSpinBox::valueChanged, this, &PaginaReporte::refrescar);
  refrescar();
}

void PaginaReporte::refrescar() {
  const int anio = anio_->value();
  const int mes = mes_->currentData().toInt();

  int movimientosDelMes = 0;
  for (const Movimiento& m : estado_->movimientos()) {
    if (m.fecha.anio == anio && m.fecha.mes == mes) ++movimientosDelMes;
  }
  const ReporteMensualPorSobre reporte = generarReporteMensualPorSobre(
      anio, mes, estado_->sobresConArchivados(),
      estado_->categoriasConArchivadas(), estado_->movimientos(),
      estado_->pasivos());

  queLleva_->setText(tr("%1: %2 movimientos repartidos en %3 sobres.")
                         .arg(QString::fromStdString(reporte.titulo))
                         .arg(movimientosDelMes)
                         .arg(reporte.sobres.size()));
}

void PaginaReporte::generar() {
  const int anio = anio_->value();
  const int mes = mes_->currentData().toInt();

  if (estado_->sobresConArchivados().empty()) {
    QMessageBox::information(this, tr("Todavía no hay nada que reportar"),
                             tr("Crea al menos un sobre y registra algún "
                                "movimiento."));
    return;
  }

  const QString sugerido = QDir(ultimaCarpeta_)
                               .filePath(QString::fromStdString(
                                   nombreDeArchivoSugerido(anio, mes)));
  const QString ruta = QFileDialog::getSaveFileName(
      this, tr("Guardar el reporte"), sugerido,
      tr("Libro de Excel (*.xlsx)"));
  if (ruta.isEmpty()) return;

  const ReporteMensualPorSobre reporte = generarReporteMensualPorSobre(
      anio, mes, estado_->sobresConArchivados(),
      estado_->categoriasConArchivadas(), estado_->movimientos(),
      estado_->pasivos());
  const xlsx::Libro libro = construirLibroDeReporte(reporte);

  const std::string problema = xlsx::escribirArchivo(libro, ruta.toStdString());
  if (!problema.empty()) {
    QMessageBox::critical(this, tr("No se pudo guardar el reporte"),
                          QString::fromStdString(problema));
    return;
  }

  ultimaCarpeta_ = QFileInfo(ruta).absolutePath();
  resultado_->setText(tr("Reporte guardado en %1 (%2 hojas).")
                          .arg(ruta)
                          .arg(libro.hojas.size()));

  const auto abrir = QMessageBox::question(
      this, tr("Reporte generado"),
      tr("Se guardó en %1.\n\n¿Quieres abrirlo?").arg(ruta));
  if (abrir == QMessageBox::Yes) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(ruta));
  }
}

}  // namespace sobres
