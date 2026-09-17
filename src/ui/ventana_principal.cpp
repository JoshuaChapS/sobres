#include "ventana_principal.h"

#include <QMessageBox>
#include <QStatusBar>

#include "comunes.h"
#include "dialogos.h"

namespace sobres {

VentanaPrincipal::VentanaPrincipal(Repositorio* repositorio, QWidget* padre)
    : QMainWindow(padre) {
  setWindowTitle(tr("Sobres"));
  resize(1040, 720);

  estado_ = new Estado(repositorio, this);

  inicio_ = new PaginaInicio(estado_);
  movimientos_ = new PaginaMovimientos(estado_);
  metas_ = new PaginaMetas(estado_);
  automatizaciones_ = new PaginaAutomatizaciones(estado_);
  reporte_ = new PaginaReporte(estado_);
  ajustes_ = new PaginaAjustes(estado_);

  pestanias_ = new QTabWidget;
  pestanias_->addTab(inicio_, tr("Inicio"));
  pestanias_->addTab(movimientos_, tr("Movimientos"));
  pestanias_->addTab(metas_, tr("Metas"));
  pestanias_->addTab(automatizaciones_, tr("Recurrentes y repartos"));
  pestanias_->addTab(reporte_, tr("Reporte"));
  pestanias_->addTab(ajustes_, tr("Ajustes"));
  setCentralWidget(pestanias_);

  connect(inicio_, &PaginaInicio::pidenNuevoSobre, this,
          &VentanaPrincipal::nuevoSobre);
  connect(inicio_, &PaginaInicio::pidenEditarSobre, this,
          &VentanaPrincipal::editarSobre);
  connect(inicio_, &PaginaInicio::pidenNuevoMovimiento, this,
          &VentanaPrincipal::nuevoMovimiento);
  connect(inicio_, &PaginaInicio::pidenVerMovimientosDe, this,
          &VentanaPrincipal::verMovimientosDe);
  connect(estado_, &Estado::cambio, this,
          &VentanaPrincipal::actualizarBarraDeEstado);

  actualizarBarraDeEstado();
}

void VentanaPrincipal::actualizarBarraDeEstado() {
  const ResumenPatrimonio& r = estado_->resumen();
  statusBar()->showMessage(
      tr("En la cuenta %1  ·  Patrimonio real %2  ·  %3 sobres activos")
          .arg(pesos(r.enLaCuenta))
          .arg(pesos(r.patrimonioReal))
          .arg(estado_->sobres().size()));
}

void VentanaPrincipal::nuevoSobre() {
  Sobre nuevo;
  const auto& colores = coloresSugeridos();
  nuevo.color =
      colores[estado_->sobresConArchivados().size() % colores.size()]
          .toStdString();
  DialogoSobre dialogo(nuevo, estado_->sobresConArchivados(), this);
  if (dialogo.exec() != QDialog::Accepted) return;
  Sobre guardado = dialogo.sobre();
  QString error;
  if (!estado_->repositorio().guardarSobre(guardado, &error)) {
    QMessageBox::critical(this, tr("No se pudo guardar"), error);
    return;
  }
  estado_->recargar();
}

void VentanaPrincipal::editarSobre(Id id) {
  for (const Sobre& s : estado_->sobresConArchivados()) {
    if (s.id != id) continue;
    DialogoSobre dialogo(s, estado_->sobresConArchivados(), this);
    if (dialogo.exec() != QDialog::Accepted) return;
    Sobre guardado = dialogo.sobre();
    QString error;
    if (!estado_->repositorio().guardarSobre(guardado, &error)) {
      QMessageBox::critical(this, tr("No se pudo guardar"), error);
      return;
    }
    estado_->recargar();
    return;
  }
}

void VentanaPrincipal::nuevoMovimiento() {
  if (estado_->sobres().empty()) {
    QMessageBox::information(
        this, tr("Falta un sobre"),
        tr("Crea al menos un sobre antes de registrar movimientos."));
    return;
  }
  Movimiento nuevo;
  nuevo.fecha = Estado::hoy();
  DialogoMovimiento dialogo(estado_, nuevo, this);
  if (dialogo.exec() != QDialog::Accepted) return;
  Movimiento guardado = dialogo.movimiento();
  QString error;
  if (!estado_->repositorio().guardarMovimiento(guardado, &error)) {
    QMessageBox::critical(this, tr("No se pudo guardar"), error);
    return;
  }
  estado_->recargar();
}

void VentanaPrincipal::verMovimientosDe(Id id) {
  movimientos_->filtrarPorSobre(id);
  pestanias_->setCurrentWidget(movimientos_);
}

}  // namespace sobres
