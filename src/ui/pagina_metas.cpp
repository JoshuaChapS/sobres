#include "pagina_metas.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>

#include "../nucleo/meta_proyeccion.h"
#include "comunes.h"
#include "dialogos.h"

namespace sobres {
namespace {

QLabel* linea(const QString& texto, int puntos, const char* color,
              bool negrita = false) {
  auto* e = new QLabel(texto);
  QFont f = e->font();
  f.setPointSize(puntos);
  f.setBold(negrita);
  e->setFont(f);
  e->setStyleSheet(QStringLiteral("color: %1;").arg(QString::fromLatin1(color)));
  e->setWordWrap(true);
  return e;
}

const char* colorDelEstado(EstadoMeta estado) {
  switch (estado) {
    case EstadoMeta::YaAlcanzada:
    case EstadoMeta::Adelantado:
      return paleta::kPositivo;
    case EstadoMeta::Atrasado:
      return paleta::kNegativo;
    case EstadoMeta::EnLinea:
    case EstadoMeta::NoProyectable:
      return paleta::kTintaSuave;
  }
  return paleta::kTintaSuave;
}

}  // namespace

PaginaMetas::PaginaMetas(Estado* estado, QWidget* padre)
    : QWidget(padre), estado_(estado) {
  auto* nueva = new QPushButton(tr("Nueva meta"));
  connect(nueva, &QPushButton::clicked, this, &PaginaMetas::nuevaMeta);

  auto* filaAcciones = new QHBoxLayout;
  filaAcciones->addWidget(nueva);
  filaAcciones->addStretch();

  aviso_ = linea(tr("Todavía no hay metas. Una meta toma el saldo de un sobre, "
                    "una fecha y una tasa esperada, y te dice cuánto tendrías "
                    "que aportar en cada periodo."),
                 10, paleta::kTintaSuave);

  auto* interior = new QWidget;
  lista_ = new QVBoxLayout(interior);
  lista_->setContentsMargins(0, 0, 0, 0);
  lista_->setSpacing(10);
  lista_->addStretch();

  auto* area = new QScrollArea;
  area->setWidgetResizable(true);
  area->setFrameShape(QFrame::NoFrame);
  area->setWidget(interior);

  auto* raiz = new QVBoxLayout(this);
  raiz->setContentsMargins(24, 20, 24, 24);
  raiz->addLayout(filaAcciones);
  raiz->addWidget(aviso_);
  raiz->addWidget(area, 1);

  connect(estado_, &Estado::cambio, this, &PaginaMetas::refrescar);
  refrescar();
}

QWidget* PaginaMetas::construirTarjeta(const Meta& meta) {
  const Centavos saldo = estado_->saldoDe(meta.sobreAsociado);
  const ProyeccionMeta p = proyectarMeta(meta, saldo, Estado::hoy());

  auto* marco = new QFrame;
  marco->setObjectName(QStringLiteral("tarjetaMeta"));
  marco->setStyleSheet(
      QStringLiteral("#tarjetaMeta { background-color: %1; border: 1px solid "
                     "%2; border-radius: 6px; }")
          .arg(QString::fromLatin1(paleta::kPapel),
               QString::fromLatin1(paleta::kLinea)));

  auto* columna = new QVBoxLayout(marco);
  columna->setContentsMargins(16, 14, 16, 14);
  columna->setSpacing(6);

  auto* encabezado = new QHBoxLayout;
  encabezado->addWidget(
      linea(QString::fromStdString(meta.nombre), 12, paleta::kTinta, true));
  encabezado->addStretch();
  auto* avanceTexto = linea(tr("%1 de %2")
                                .arg(pesos(saldo))
                                .arg(pesos(meta.montoObjetivo)),
                            11, paleta::kTinta);
  avanceTexto->setWordWrap(false);
  encabezado->addWidget(avanceTexto);
  columna->addLayout(encabezado);

  auto* barra = new QProgressBar;
  barra->setRange(0, 1000);
  const double avance =
      meta.montoObjetivo > 0
          ? 1000.0 * static_cast<double>(saldo) /
                static_cast<double>(meta.montoObjetivo)
          : 0.0;
  barra->setValue(static_cast<int>(avance < 0 ? 0 : (avance > 1000 ? 1000 : avance)));
  barra->setTextVisible(false);
  barra->setFixedHeight(6);
  barra->setStyleSheet(
      QStringLiteral("QProgressBar { background-color: #eceff1; border: none; "
                     "border-radius: 3px; } QProgressBar::chunk { "
                     "background-color: %1; border-radius: 3px; }")
          .arg(estado_->colorDeSobre(meta.sobreAsociado)));
  columna->addWidget(barra);

  columna->addWidget(linea(
      tr("Sobre %1 · fecha objetivo %2 · tasa anual esperada %3%")
          .arg(estado_->nombreDeSobre(meta.sobreAsociado))
          .arg(QString::fromStdString(aTextoLargo(meta.fechaObjetivo)))
          .arg(QString::number(meta.tasaAnual * 100.0, 'f', 2)),
      9, paleta::kTintaSuave));

  columna->addWidget(
      linea(QString::fromStdString(resumirProyeccion(p, meta.periodicidad)), 10,
            colorDelEstado(p.estado)));

  if (p.proyectable) {
    QString detalle =
        tr("Faltan %1 periodos. Aporte requerido %2, aporte planeado %3.")
            .arg(p.periodos)
            .arg(pesos(p.aporteRequerido))
            .arg(pesos(meta.aportePlaneado));
    if (p.tieneFechaEstimada && p.estado != EstadoMeta::YaAlcanzada) {
      detalle += tr(" Al ritmo planeado la alcanzas el %1.")
                     .arg(QString::fromStdString(aTextoLargo(p.fechaEstimada)));
    }
    columna->addWidget(linea(detalle, 9, paleta::kTintaSuave));
  }

  auto* acciones = new QHBoxLayout;
  acciones->addStretch();
  auto* aportar = new QPushButton(tr("Registrar aporte"));
  auto* editar = new QPushButton(tr("Editar"));
  auto* borrar = new QPushButton(tr("Eliminar"));
  acciones->addWidget(aportar);
  acciones->addWidget(editar);
  acciones->addWidget(borrar);
  columna->addLayout(acciones);

  const Id idMeta = meta.id;
  const Meta copia = meta;

  connect(aportar, &QPushButton::clicked, this, [this, copia]() {
    Movimiento aporte;
    aporte.fecha = Estado::hoy();
    aporte.tipo = TipoMovimiento::Entrada;
    aporte.sobreOrigen = copia.sobreAsociado;
    aporte.monto = copia.aportePlaneado;
    aporte.nota = "Aporte a " + copia.nombre;
    DialogoMovimiento dialogo(estado_, aporte, this);
    if (dialogo.exec() != QDialog::Accepted) return;
    Movimiento guardado = dialogo.movimiento();
    QString error;
    if (!estado_->repositorio().guardarMovimiento(guardado, &error)) {
      QMessageBox::critical(this, tr("No se pudo guardar"), error);
      return;
    }
    estado_->recargar();
  });

  connect(editar, &QPushButton::clicked, this, [this, copia]() {
    DialogoMeta dialogo(estado_, copia, this);
    if (dialogo.exec() != QDialog::Accepted) return;
    Meta guardada = dialogo.meta();
    QString error;
    if (!estado_->repositorio().guardarMeta(guardada, &error)) {
      QMessageBox::critical(this, tr("No se pudo guardar"), error);
      return;
    }
    estado_->recargar();
  });

  connect(borrar, &QPushButton::clicked, this, [this, idMeta]() {
    const auto respuesta = QMessageBox::question(
        this, tr("Eliminar meta"),
        tr("¿Eliminar esta meta? El sobre y su dinero se quedan como están."));
    if (respuesta != QMessageBox::Yes) return;
    QString error;
    if (!estado_->repositorio().eliminarMeta(idMeta, &error)) {
      QMessageBox::critical(this, tr("No se pudo eliminar"), error);
      return;
    }
    estado_->recargar();
  });

  return marco;
}

void PaginaMetas::refrescar() {
  while (lista_->count() > 0) {
    QLayoutItem* item = lista_->takeAt(0);
    if (QWidget* w = item->widget()) w->deleteLater();
    delete item;
  }
  for (const Meta& m : estado_->metas()) {
    lista_->addWidget(construirTarjeta(m));
  }
  lista_->addStretch();
  aviso_->setVisible(estado_->metas().empty());
}

void PaginaMetas::nuevaMeta() {
  if (estado_->sobres().empty()) {
    QMessageBox::information(
        this, tr("Falta un sobre"),
        tr("Una meta necesita un sobre donde se junte el dinero. Crea primero "
           "el sobre."));
    return;
  }
  Meta nueva;
  nueva.fechaObjetivo = sumarMeses(Estado::hoy(), 12);
  DialogoMeta dialogo(estado_, nueva, this);
  if (dialogo.exec() != QDialog::Accepted) return;
  Meta guardada = dialogo.meta();
  QString error;
  if (!estado_->repositorio().guardarMeta(guardada, &error)) {
    QMessageBox::critical(this, tr("No se pudo guardar"), error);
    return;
  }
  estado_->recargar();
}

}  // namespace sobres
