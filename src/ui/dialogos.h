// Sobres — dialogos.h
// Las ventanas de alta y edición. Cada una valida con las mismas reglas del
// núcleo que usa la capa de datos, así que no hay dos criterios distintos.
#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>

#include "comunes.h"
#include "estado.h"

namespace sobres {

// Botón que muestra un color y abre el selector al pulsarlo.
class BotonColor : public QPushButton {
  Q_OBJECT

 public:
  explicit BotonColor(QWidget* padre = nullptr);
  void fijarColor(const QString& color);
  QString color() const { return color_; }

 private:
  void actualizarApariencia();
  QString color_ = QStringLiteral("#4b6bfb");
};

class DialogoSobre : public QDialog {
  Q_OBJECT

 public:
  DialogoSobre(const Sobre& inicial, std::vector<Sobre> existentes,
               QWidget* padre = nullptr);
  Sobre sobre() const { return sobre_; }

 private slots:
  void aceptar();

 private:
  Sobre sobre_;
  std::vector<Sobre> existentes_;
  QLineEdit* nombre_;
  QComboBox* tipo_;
  BotonColor* color_;
};

class DialogoCategoria : public QDialog {
  Q_OBJECT

 public:
  DialogoCategoria(const Categoria& inicial, std::vector<Categoria> existentes,
                   QWidget* padre = nullptr);
  Categoria categoria() const { return categoria_; }

 private slots:
  void aceptar();

 private:
  Categoria categoria_;
  std::vector<Categoria> existentes_;
  QLineEdit* nombre_;
  QLineEdit* icono_;
  BotonColor* color_;
};

class DialogoMovimiento : public QDialog {
  Q_OBJECT

 public:
  DialogoMovimiento(Estado* estado, const Movimiento& inicial,
                    QWidget* padre = nullptr);
  Movimiento movimiento() const { return movimiento_; }

 private slots:
  void aceptar();
  void ajustarSegunTipo();

 private:
  Estado* estado_;
  Movimiento movimiento_;
  QComboBox* tipo_;
  QDateEdit* fecha_;
  QComboBox* sobreOrigen_;
  QComboBox* sobreDestino_;
  CampoCategoria* categoria_;
  CampoMonto* monto_;
  QLineEdit* nota_;
  QLabel* etiquetaOrigen_;
  QLabel* etiquetaDestino_;
  QLabel* etiquetaCategoria_;
};

class DialogoPasivo : public QDialog {
  Q_OBJECT

 public:
  explicit DialogoPasivo(const Pasivo& inicial, QWidget* padre = nullptr);
  Pasivo pasivo() const { return pasivo_; }

 private slots:
  void aceptar();

 private:
  Pasivo pasivo_;
  QLineEdit* nombre_;
  CampoMonto* saldo_;
  QLineEdit* nota_;
};

class DialogoMeta : public QDialog {
  Q_OBJECT

 public:
  DialogoMeta(Estado* estado, const Meta& inicial, QWidget* padre = nullptr);
  Meta meta() const { return meta_; }

 private slots:
  void aceptar();
  void actualizarVistaPrevia();

 private:
  Estado* estado_;
  Meta meta_;
  QLineEdit* nombre_;
  CampoMonto* objetivo_;
  QDateEdit* fechaObjetivo_;
  QComboBox* sobre_;
  CampoMonto* aporte_;
  QComboBox* periodicidad_;
  QDoubleSpinBox* tasa_;
  QLineEdit* nota_;
  QLabel* vistaPrevia_;
};

class DialogoRecurrente : public QDialog {
  Q_OBJECT

 public:
  DialogoRecurrente(Estado* estado, const Recurrente& inicial,
                    QWidget* padre = nullptr);
  Recurrente recurrente() const { return recurrente_; }

 private slots:
  void aceptar();
  void ajustarSegunTipo();

 private:
  Estado* estado_;
  Recurrente recurrente_;
  QLineEdit* nombre_;
  QComboBox* tipo_;
  QComboBox* sobreOrigen_;
  QComboBox* sobreDestino_;
  CampoCategoria* categoria_;
  CampoMonto* monto_;
  QComboBox* periodicidad_;
  QDateEdit* proximaFecha_;
  QCheckBox* activo_;
  QLineEdit* nota_;
  QLabel* etiquetaDestino_;
};

class DialogoPlantilla : public QDialog {
  Q_OBJECT

 public:
  DialogoPlantilla(Estado* estado, const PlantillaReparto& inicial,
                   QWidget* padre = nullptr);
  PlantillaReparto plantilla() const { return plantilla_; }

 private slots:
  void aceptar();
  void agregarLinea();
  void quitarLinea();
  void actualizarSuma();

 private:
  void ponerLinea(int fila, const LineaReparto& linea);
  PlantillaReparto leerDeLaTabla() const;

  Estado* estado_;
  PlantillaReparto plantilla_;
  QLineEdit* nombre_;
  QComboBox* modo_;
  CampoMonto* montoSugerido_;
  QComboBox* sobreFuente_;
  CampoCategoria* categoria_;
  QTableWidget* tabla_;
  QLabel* suma_;
};

// Pregunta el monto y la fecha con los que se aplica una plantilla, y enseña
// cómo queda repartido antes de confirmar.
class DialogoAplicarReparto : public QDialog {
  Q_OBJECT

 public:
  DialogoAplicarReparto(Estado* estado, const PlantillaReparto& plantilla,
                        QWidget* padre = nullptr);
  Fecha fecha() const;
  Centavos montoTotal() const;

 private slots:
  void actualizarVistaPrevia();
  void aceptar();

 private:
  Estado* estado_;
  PlantillaReparto plantilla_;
  QDateEdit* fecha_;
  CampoMonto* monto_;
  QTableWidget* vistaPrevia_;
  QLabel* aviso_;
};

}  // namespace sobres
