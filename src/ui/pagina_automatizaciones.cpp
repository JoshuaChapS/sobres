#include "pagina_automatizaciones.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "../nucleo/recurrencia.h"
#include "../nucleo/reparto.h"
#include "comunes.h"
#include "dialogos.h"

namespace sobres {
namespace {

constexpr int kColumnaId = 0;

Id idDeFila(const QTableWidget* tabla) {
  const int fila = tabla->currentRow();
  if (fila < 0) return kSinId;
  const QTableWidgetItem* celda = tabla->item(fila, kColumnaId);
  return celda ? static_cast<Id>(celda->text().toLongLong()) : kSinId;
}

}  // namespace

PaginaAutomatizaciones::PaginaAutomatizaciones(Estado* estado, QWidget* padre)
    : QWidget(padre), estado_(estado) {
  // --- Recurrentes ---------------------------------------------------------
  tablaRecurrentes_ = new QTableWidget(0, 7);
  tablaRecurrentes_->setHorizontalHeaderLabels(
      {tr("id"), tr("Nombre"), tr("Tipo"), tr("Sobre"), tr("Monto"),
       tr("Cada"), tr("Próxima")});
  tablaRecurrentes_->setColumnHidden(kColumnaId, true);
  tablaRecurrentes_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tablaRecurrentes_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tablaRecurrentes_->setSelectionMode(QAbstractItemView::SingleSelection);
  tablaRecurrentes_->verticalHeader()->setVisible(false);
  tablaRecurrentes_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);

  auto* aplicarRecurrente = new QPushButton(tr("Registrar el de esta vez"));
  auto* nuevoRec = new QPushButton(tr("Nuevo"));
  auto* editarRec = new QPushButton(tr("Editar"));
  auto* borrarRec = new QPushButton(tr("Eliminar"));
  connect(aplicarRecurrente, &QPushButton::clicked, this,
          &PaginaAutomatizaciones::materializarRecurrente);
  connect(nuevoRec, &QPushButton::clicked, this,
          &PaginaAutomatizaciones::nuevoRecurrente);
  connect(editarRec, &QPushButton::clicked, this,
          &PaginaAutomatizaciones::editarRecurrente);
  connect(borrarRec, &QPushButton::clicked, this,
          &PaginaAutomatizaciones::eliminarRecurrente);

  avisoPendientes_ = new QLabel;
  avisoPendientes_->setWordWrap(true);
  avisoPendientes_->setStyleSheet(
      QStringLiteral("color: %1;").arg(QString::fromLatin1(paleta::kTintaSuave)));

  auto* accionesRec = new QHBoxLayout;
  accionesRec->addWidget(aplicarRecurrente);
  accionesRec->addWidget(nuevoRec);
  accionesRec->addWidget(editarRec);
  accionesRec->addWidget(borrarRec);
  accionesRec->addStretch();

  auto* grupoRec = new QGroupBox(tr("Movimientos recurrentes"));
  auto* columnaRec = new QVBoxLayout(grupoRec);
  columnaRec->addLayout(accionesRec);
  columnaRec->addWidget(tablaRecurrentes_);
  columnaRec->addWidget(avisoPendientes_);

  // --- Plantillas de reparto ----------------------------------------------
  tablaPlantillas_ = new QTableWidget(0, 5);
  tablaPlantillas_->setHorizontalHeaderLabels(
      {tr("id"), tr("Nombre"), tr("Modo"), tr("Líneas"), tr("Sale de")});
  tablaPlantillas_->setColumnHidden(kColumnaId, true);
  tablaPlantillas_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tablaPlantillas_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tablaPlantillas_->setSelectionMode(QAbstractItemView::SingleSelection);
  tablaPlantillas_->verticalHeader()->setVisible(false);
  tablaPlantillas_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);

  auto* aplicar = new QPushButton(tr("Aplicar reparto"));
  auto* nuevaPla = new QPushButton(tr("Nueva"));
  auto* editarPla = new QPushButton(tr("Editar"));
  auto* borrarPla = new QPushButton(tr("Eliminar"));
  connect(aplicar, &QPushButton::clicked, this,
          &PaginaAutomatizaciones::aplicarPlantilla);
  connect(nuevaPla, &QPushButton::clicked, this,
          &PaginaAutomatizaciones::nuevaPlantilla);
  connect(editarPla, &QPushButton::clicked, this,
          &PaginaAutomatizaciones::editarPlantilla);
  connect(borrarPla, &QPushButton::clicked, this,
          &PaginaAutomatizaciones::eliminarPlantilla);

  auto* accionesPla = new QHBoxLayout;
  accionesPla->addWidget(aplicar);
  accionesPla->addWidget(nuevaPla);
  accionesPla->addWidget(editarPla);
  accionesPla->addWidget(borrarPla);
  accionesPla->addStretch();

  auto* grupoPla = new QGroupBox(tr("Plantillas de reparto"));
  auto* columnaPla = new QVBoxLayout(grupoPla);
  columnaPla->addLayout(accionesPla);
  columnaPla->addWidget(tablaPlantillas_);

  auto* raiz = new QVBoxLayout(this);
  raiz->setContentsMargins(24, 20, 24, 24);
  raiz->addWidget(grupoRec);
  raiz->addWidget(grupoPla);
  raiz->addStretch();

  connect(estado_, &Estado::cambio, this,
          &PaginaAutomatizaciones::refrescar);
  refrescar();
}

void PaginaAutomatizaciones::refrescar() {
  llenarRecurrentes();
  llenarPlantillas();
}

void PaginaAutomatizaciones::llenarRecurrentes() {
  tablaRecurrentes_->setRowCount(0);
  int pendientes = 0;
  const Fecha hoy = Estado::hoy();

  for (const Recurrente& r : estado_->recurrentes()) {
    const int fila = tablaRecurrentes_->rowCount();
    tablaRecurrentes_->insertRow(fila);
    tablaRecurrentes_->setItem(fila, kColumnaId,
                               new QTableWidgetItem(QString::number(r.id)));

    QString nombre = QString::fromStdString(r.nombre);
    if (!r.activo) nombre += tr("  (pausado)");
    tablaRecurrentes_->setItem(fila, 1, new QTableWidgetItem(nombre));
    tablaRecurrentes_->setItem(
        fila, 2,
        new QTableWidgetItem(QString::fromStdString(
            nombreDeTipoMovimiento(r.tipo))));
    QString sobre = estado_->nombreDeSobre(r.sobreOrigen);
    if (r.tipo == TipoMovimiento::Traspaso) {
      sobre += QStringLiteral(" → ") + estado_->nombreDeSobre(r.sobreDestino);
    }
    tablaRecurrentes_->setItem(fila, 3, new QTableWidgetItem(sobre));
    auto* celdaMonto = new QTableWidgetItem(pesos(r.monto));
    celdaMonto->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    tablaRecurrentes_->setItem(fila, 4, celdaMonto);
    tablaRecurrentes_->setItem(
        fila, 5,
        new QTableWidgetItem(QString::fromStdString(
            nombreDePeriodicidad(r.periodicidad))));

    const long long vencidos = materializacionesPendientes(r, hoy);
    QString proxima = aQDate(r.proximaFecha).toString(QStringLiteral("dd/MM/yyyy"));
    if (vencidos > 0) {
      proxima += tr("  · %1 por registrar").arg(vencidos);
      pendientes += static_cast<int>(vencidos);
    }
    tablaRecurrentes_->setItem(fila, 6, new QTableWidgetItem(proxima));
  }
  tablaRecurrentes_->resizeColumnsToContents();
  tablaRecurrentes_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);
  ajustarAlturaDeTabla(tablaRecurrentes_, 8, 3);

  if (estado_->recurrentes().empty()) {
    avisoPendientes_->setText(
        tr("Aquí van los movimientos que se repiten. La aplicación nunca los "
           "registra sola: te avisa cuándo toca y tú confirmas."));
  } else if (pendientes == 0) {
    avisoPendientes_->setText(tr("No hay recurrentes vencidos."));
  } else {
    avisoPendientes_->setText(
        tr("Hay %1 ocurrencias vencidas por registrar. Selecciona un "
           "recurrente y pulsa «Registrar el de esta vez» una vez por cada "
           "ocurrencia.")
            .arg(pendientes));
  }
}

void PaginaAutomatizaciones::llenarPlantillas() {
  tablaPlantillas_->setRowCount(0);
  for (const PlantillaReparto& p : estado_->plantillas()) {
    const int fila = tablaPlantillas_->rowCount();
    tablaPlantillas_->insertRow(fila);
    tablaPlantillas_->setItem(fila, kColumnaId,
                              new QTableWidgetItem(QString::number(p.id)));
    tablaPlantillas_->setItem(
        fila, 1, new QTableWidgetItem(QString::fromStdString(p.nombre)));
    tablaPlantillas_->setItem(
        fila, 2,
        new QTableWidgetItem(p.modo == ModoReparto::Porcentajes
                                 ? tr("Porcentajes")
                                 : tr("Montos fijos")));
    tablaPlantillas_->setItem(
        fila, 3,
        new QTableWidgetItem(tr("%1 sobres").arg(p.lineas.size())));
    tablaPlantillas_->setItem(
        fila, 4,
        new QTableWidgetItem(p.sobreFuente == kSinId
                                 ? tr("Dinero que llega de fuera")
                                 : estado_->nombreDeSobre(p.sobreFuente)));
  }
  tablaPlantillas_->resizeColumnsToContents();
  tablaPlantillas_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::Stretch);
  ajustarAlturaDeTabla(tablaPlantillas_, 8, 3);
}

Id PaginaAutomatizaciones::recurrenteSeleccionado() const {
  return idDeFila(tablaRecurrentes_);
}

Id PaginaAutomatizaciones::plantillaSeleccionada() const {
  return idDeFila(tablaPlantillas_);
}

void PaginaAutomatizaciones::nuevoRecurrente() {
  if (estado_->sobres().empty()) {
    QMessageBox::information(this, tr("Falta un sobre"),
                             tr("Crea al menos un sobre antes."));
    return;
  }
  Recurrente nuevo;
  nuevo.proximaFecha = Estado::hoy();
  DialogoRecurrente dialogo(estado_, nuevo, this);
  if (dialogo.exec() != QDialog::Accepted) return;
  Recurrente guardado = dialogo.recurrente();
  QString error;
  if (!estado_->repositorio().guardarRecurrente(guardado, &error)) {
    QMessageBox::critical(this, tr("No se pudo guardar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAutomatizaciones::editarRecurrente() {
  const Id id = recurrenteSeleccionado();
  if (id == kSinId) return;
  for (const Recurrente& r : estado_->recurrentes()) {
    if (r.id != id) continue;
    DialogoRecurrente dialogo(estado_, r, this);
    if (dialogo.exec() != QDialog::Accepted) return;
    Recurrente guardado = dialogo.recurrente();
    QString error;
    if (!estado_->repositorio().guardarRecurrente(guardado, &error)) {
      QMessageBox::critical(this, tr("No se pudo guardar"), error);
      return;
    }
    estado_->recargar();
    return;
  }
}

void PaginaAutomatizaciones::eliminarRecurrente() {
  const Id id = recurrenteSeleccionado();
  if (id == kSinId) return;
  const auto respuesta = QMessageBox::question(
      this, tr("Eliminar recurrente"),
      tr("¿Eliminar esta plantilla? Los movimientos que ya registraste se "
         "quedan como están."));
  if (respuesta != QMessageBox::Yes) return;
  QString error;
  if (!estado_->repositorio().eliminarRecurrente(id, &error)) {
    QMessageBox::critical(this, tr("No se pudo eliminar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAutomatizaciones::materializarRecurrente() {
  const Id id = recurrenteSeleccionado();
  if (id == kSinId) {
    QMessageBox::information(this, tr("Selecciona un recurrente"),
                             tr("Elige de la lista el que quieres registrar."));
    return;
  }
  Recurrente elegido;
  bool hallado = false;
  for (const Recurrente& r : estado_->recurrentes()) {
    if (r.id == id) {
      elegido = r;
      hallado = true;
    }
  }
  if (!hallado) return;

  const auto respuesta = QMessageBox::question(
      this, tr("Registrar movimiento"),
      tr("¿Registrar «%1» por %2 con fecha %3?")
          .arg(QString::fromStdString(elegido.nombre))
          .arg(pesos(elegido.monto))
          .arg(QString::fromStdString(aTextoLargo(elegido.proximaFecha))));
  if (respuesta != QMessageBox::Yes) return;

  QString error;
  // Se registra con la fecha programada, no con la de hoy: si se aplica tarde,
  // el gasto sigue perteneciendo al mes que le tocaba.
  if (!estado_->repositorio().materializarRecurrente(id, elegido.proximaFecha,
                                                     &error)) {
    QMessageBox::critical(this, tr("No se pudo registrar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAutomatizaciones::nuevaPlantilla() {
  if (estado_->sobres().empty()) {
    QMessageBox::information(this, tr("Falta un sobre"),
                             tr("Crea al menos un sobre antes."));
    return;
  }
  PlantillaReparto nueva;
  DialogoPlantilla dialogo(estado_, nueva, this);
  if (dialogo.exec() != QDialog::Accepted) return;
  PlantillaReparto guardada = dialogo.plantilla();
  QString error;
  if (!estado_->repositorio().guardarPlantilla(guardada, &error)) {
    QMessageBox::critical(this, tr("No se pudo guardar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAutomatizaciones::editarPlantilla() {
  const Id id = plantillaSeleccionada();
  if (id == kSinId) return;
  for (const PlantillaReparto& p : estado_->plantillas()) {
    if (p.id != id) continue;
    DialogoPlantilla dialogo(estado_, p, this);
    if (dialogo.exec() != QDialog::Accepted) return;
    PlantillaReparto guardada = dialogo.plantilla();
    QString error;
    if (!estado_->repositorio().guardarPlantilla(guardada, &error)) {
      QMessageBox::critical(this, tr("No se pudo guardar"), error);
      return;
    }
    estado_->recargar();
    return;
  }
}

void PaginaAutomatizaciones::eliminarPlantilla() {
  const Id id = plantillaSeleccionada();
  if (id == kSinId) return;
  const auto respuesta =
      QMessageBox::question(this, tr("Eliminar plantilla"),
                            tr("¿Eliminar esta plantilla de reparto?"));
  if (respuesta != QMessageBox::Yes) return;
  QString error;
  if (!estado_->repositorio().eliminarPlantilla(id, &error)) {
    QMessageBox::critical(this, tr("No se pudo eliminar"), error);
    return;
  }
  estado_->recargar();
}

void PaginaAutomatizaciones::aplicarPlantilla() {
  const Id id = plantillaSeleccionada();
  if (id == kSinId) {
    QMessageBox::information(this, tr("Selecciona una plantilla"),
                             tr("Elige de la lista la que quieres aplicar."));
    return;
  }
  for (const PlantillaReparto& p : estado_->plantillas()) {
    if (p.id != id) continue;
    DialogoAplicarReparto dialogo(estado_, p, this);
    if (dialogo.exec() != QDialog::Accepted) return;

    const ResultadoReparto resultado =
        calcularReparto(p, dialogo.montoTotal());
    std::vector<Movimiento> movimientos =
        generarMovimientosDeReparto(p, resultado, dialogo.fecha());
    QString error;
    if (!estado_->repositorio().guardarMovimientos(movimientos, &error)) {
      QMessageBox::critical(this, tr("No se pudo aplicar"), error);
      return;
    }
    estado_->recargar();
    QMessageBox::information(
        this, tr("Reparto aplicado"),
        tr("Se registraron %1 movimientos por un total de %2.")
            .arg(movimientos.size())
            .arg(pesos(resultado.total)));
    return;
  }
}

}  // namespace sobres
