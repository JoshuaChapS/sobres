// Sobres — pagina_inicio.h
// La pantalla principal. Arriba, las dos cifras que no son lo mismo: lo que
// dice el banco y lo que de verdad es del usuario. Abajo, los sobres.
#pragma once

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include "estado.h"

namespace sobres {

// La tarjeta de un sobre en la pantalla principal.
class TarjetaSobre : public QFrame {
  Q_OBJECT

 public:
  TarjetaSobre(const Sobre& sobre, Centavos saldo, QWidget* padre = nullptr);

 signals:
  void editar(Id id);
  void archivar(Id id);
  void mover(Id id, int desplazamiento);
  void verMovimientos(Id id);

 private:
  Id id_;
};

class PaginaInicio : public QWidget {
  Q_OBJECT

 public:
  explicit PaginaInicio(Estado* estado, QWidget* padre = nullptr);

 signals:
  // La pantalla principal no toca la base directamente: pide a la ventana que
  // abra el diálogo correspondiente.
  void pidenNuevoSobre();
  void pidenEditarSobre(Id id);
  void pidenNuevoMovimiento();
  void pidenVerMovimientosDe(Id id);

 public slots:
  void refrescar();

 private:
  void construirCabecera(QVBoxLayout* raiz);

  Estado* estado_;
  QLabel* cifraCuenta_;
  QLabel* cifraPatrimonio_;
  QLabel* explicacion_;
  QWidget* contenedorSobres_;
  QGridLayout* rejillaSobres_;
  QWidget* panelVacio_;
  QWidget* panelConSobres_;
};

}  // namespace sobres
