// Sobres — pagina_reporte.h
// El reporte no se lee dentro de la aplicación: se elige un mes y se genera un
// archivo de Excel con una hoja por sobre.
#pragma once

#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QWidget>

#include "estado.h"

namespace sobres {

class PaginaReporte : public QWidget {
  Q_OBJECT

 public:
  explicit PaginaReporte(Estado* estado, QWidget* padre = nullptr);

 public slots:
  void refrescar();

 private slots:
  void generar();

 private:
  Estado* estado_;
  QComboBox* mes_;
  QSpinBox* anio_;
  QLabel* queLleva_;
  QLabel* resultado_;
  QString ultimaCarpeta_;
};

}  // namespace sobres
