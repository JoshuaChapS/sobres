// Sobres — ventana_principal.h
#pragma once

#include <QMainWindow>
#include <QTabWidget>

#include "estado.h"
#include "pagina_ajustes.h"
#include "pagina_automatizaciones.h"
#include "pagina_inicio.h"
#include "pagina_metas.h"
#include "pagina_movimientos.h"
#include "pagina_reporte.h"

namespace sobres {

class VentanaPrincipal : public QMainWindow {
  Q_OBJECT

 public:
  explicit VentanaPrincipal(Repositorio* repositorio, QWidget* padre = nullptr);

 private slots:
  void nuevoSobre();
  void editarSobre(Id id);
  void nuevoMovimiento();
  void verMovimientosDe(Id id);
  void actualizarBarraDeEstado();

 private:
  Estado* estado_;
  QTabWidget* pestanias_;
  PaginaInicio* inicio_;
  PaginaMovimientos* movimientos_;
  PaginaMetas* metas_;
  PaginaAutomatizaciones* automatizaciones_;
  PaginaReporte* reporte_;
  PaginaAjustes* ajustes_;
};

}  // namespace sobres
