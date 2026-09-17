#include "pagina_inicio.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>

#include "comunes.h"

namespace sobres {
namespace {

QLabel* etiqueta(const QString& texto, int puntos, const char* color,
                 bool negrita = false) {
  auto* e = new QLabel(texto);
  QFont fuente = e->font();
  fuente.setPointSize(puntos);
  fuente.setBold(negrita);
  e->setFont(fuente);
  e->setStyleSheet(QStringLiteral("color: %1;").arg(QString::fromLatin1(color)));
  return e;
}

}  // namespace

// ---------------------------------------------------------------------------
// TarjetaSobre
// ---------------------------------------------------------------------------

TarjetaSobre::TarjetaSobre(const Sobre& sobre, Centavos saldo, QWidget* padre)
    : QFrame(padre), id_(sobre.id) {
  const bool ajeno = sobre.tipo == TipoSobre::Ajeno;
  // Sin esto, una hoja de estilo no pinta el fondo ni el borde de un widget
  // derivado de QFrame.
  setAttribute(Qt::WA_StyledBackground, true);
  // Se usa el nombre del objeto y no el de la clase porque en una hoja de
  // estilo de Qt el nombre de una clase dentro de un espacio de nombres se
  // escribiría "sobres--TarjetaSobre", que es fácil de romper sin notarlo.
  setObjectName(QStringLiteral("tarjetaSobre"));

  // El dinero ajeno se distingue por la forma, no solo por el color: borde
  // punteado y fondo apagado. Así se nota aunque se imprima en blanco y negro.
  setStyleSheet(
      QStringLiteral(
          "#tarjetaSobre { background-color: %1; border: %2px %3 %4; "
          "border-radius: 6px; }")
          .arg(QString::fromLatin1(ajeno ? "#efeeea" : paleta::kPapel))
          .arg(ajeno ? 2 : 1)
          .arg(QString::fromLatin1(ajeno ? "dashed" : "solid"),
               QString::fromLatin1(ajeno ? "#a8a49b" : paleta::kLinea)));

  auto* franja = new QFrame;
  franja->setFixedWidth(4);
  franja->setStyleSheet(
      QStringLiteral("background-color: %1; border: none; border-radius: 2px;")
          .arg(QString::fromStdString(sobre.color)));

  auto* nombre = etiqueta(QString::fromStdString(sobre.nombre), 11,
                          paleta::kTinta, true);
  QString subtitulo = QString::fromStdString(nombreDeTipo(sobre.tipo));
  if (ajeno) subtitulo = tr("De terceros — no suma a tu patrimonio");
  if (sobre.tipo == TipoSobre::Meta) subtitulo = tr("Apartado para una meta");
  if (sobre.tipo == TipoSobre::Propio) subtitulo = tr("Disponible");
  auto* tipo = etiqueta(subtitulo, 8, paleta::kTintaSuave);

  auto* columnaTexto = new QVBoxLayout;
  columnaTexto->setSpacing(2);
  columnaTexto->addWidget(nombre);
  columnaTexto->addWidget(tipo);

  auto* montoEtiqueta =
      etiqueta(pesos(saldo), 14,
               saldo < 0 ? paleta::kNegativo : paleta::kTinta, true);
  montoEtiqueta->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  auto* subir = new QToolButton;
  subir->setText(QStringLiteral("↑"));
  subir->setToolTip(tr("Subir en la lista"));
  auto* bajar = new QToolButton;
  bajar->setText(QStringLiteral("↓"));
  bajar->setToolTip(tr("Bajar en la lista"));
  auto* botonEditar = new QToolButton;
  botonEditar->setText(tr("Editar"));
  auto* botonArchivar = new QToolButton;
  botonArchivar->setText(tr("Archivar"));
  auto* botonVer = new QToolButton;
  botonVer->setText(tr("Movimientos"));

  for (QToolButton* boton : {subir, bajar, botonEditar, botonArchivar, botonVer}) {
    boton->setAutoRaise(true);
    boton->setStyleSheet(
        QStringLiteral("QToolButton { color: %1; border: none; padding: 2px "
                       "6px; } QToolButton:hover { color: %2; }")
            .arg(QString::fromLatin1(paleta::kTintaSuave),
                 QString::fromLatin1(paleta::kTinta)));
  }

  connect(subir, &QToolButton::clicked, this, [this]() { emit mover(id_, -1); });
  connect(bajar, &QToolButton::clicked, this, [this]() { emit mover(id_, 1); });
  connect(botonEditar, &QToolButton::clicked, this,
          [this]() { emit editar(id_); });
  connect(botonArchivar, &QToolButton::clicked, this,
          [this]() { emit archivar(id_); });
  connect(botonVer, &QToolButton::clicked, this,
          [this]() { emit verMovimientos(id_); });

  auto* botones = new QHBoxLayout;
  botones->setSpacing(0);
  botones->addStretch();
  botones->addWidget(botonVer);
  botones->addWidget(botonEditar);
  botones->addWidget(botonArchivar);
  botones->addWidget(subir);
  botones->addWidget(bajar);

  auto* columnaDerecha = new QVBoxLayout;
  columnaDerecha->setSpacing(2);
  columnaDerecha->addWidget(montoEtiqueta);
  columnaDerecha->addLayout(botones);

  auto* fila = new QHBoxLayout(this);
  fila->setContentsMargins(10, 10, 10, 10);
  fila->addWidget(franja);
  fila->addSpacing(8);
  fila->addLayout(columnaTexto, 1);
  fila->addLayout(columnaDerecha);
}

// ---------------------------------------------------------------------------
// PaginaInicio
// ---------------------------------------------------------------------------

PaginaInicio::PaginaInicio(Estado* estado, QWidget* padre)
    : QWidget(padre), estado_(estado) {
  auto* raiz = new QVBoxLayout(this);
  raiz->setContentsMargins(0, 0, 0, 0);

  auto* area = new QScrollArea;
  area->setWidgetResizable(true);
  area->setFrameShape(QFrame::NoFrame);
  area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto* interior = new QWidget;
  auto* columna = new QVBoxLayout(interior);
  columna->setContentsMargins(24, 20, 24, 24);
  columna->setSpacing(16);

  construirCabecera(columna);

  // Panel de onboarding: la aplicación arranca sin ningún sobre.
  panelVacio_ = new QWidget;
  auto* vacio = new QVBoxLayout(panelVacio_);
  vacio->setContentsMargins(0, 24, 0, 0);
  vacio->addWidget(etiqueta(tr("Todavía no hay sobres"), 13, paleta::kTinta,
                            true));
  auto* explicacionVacio = etiqueta(
      tr("Un sobre es una bolsa dentro de tu cuenta. Empieza por el dinero que "
         "usas para el día a día, y crea otro aparte para lo que en realidad "
         "es de alguien más."),
      10, paleta::kTintaSuave);
  explicacionVacio->setWordWrap(true);
  vacio->addWidget(explicacionVacio);
  auto* botonPrimero = new QPushButton(tr("Crear mi primer sobre"));
  botonPrimero->setFixedWidth(220);
  connect(botonPrimero, &QPushButton::clicked, this,
          &PaginaInicio::pidenNuevoSobre);
  vacio->addSpacing(8);
  vacio->addWidget(botonPrimero);
  vacio->addStretch();
  columna->addWidget(panelVacio_);

  panelConSobres_ = new QWidget;
  auto* conSobres = new QVBoxLayout(panelConSobres_);
  conSobres->setContentsMargins(0, 0, 0, 0);

  auto* filaAcciones = new QHBoxLayout;
  auto* nuevoMovimiento = new QPushButton(tr("Registrar movimiento"));
  auto* nuevoSobre = new QPushButton(tr("Nuevo sobre"));
  connect(nuevoMovimiento, &QPushButton::clicked, this,
          &PaginaInicio::pidenNuevoMovimiento);
  connect(nuevoSobre, &QPushButton::clicked, this,
          &PaginaInicio::pidenNuevoSobre);
  filaAcciones->addWidget(nuevoMovimiento);
  filaAcciones->addWidget(nuevoSobre);
  filaAcciones->addStretch();
  conSobres->addLayout(filaAcciones);
  conSobres->addSpacing(12);

  contenedorSobres_ = new QWidget;
  rejillaSobres_ = new QGridLayout(contenedorSobres_);
  rejillaSobres_->setContentsMargins(0, 0, 0, 0);
  rejillaSobres_->setSpacing(10);
  conSobres->addWidget(contenedorSobres_);
  conSobres->addStretch();
  columna->addWidget(panelConSobres_);

  area->setWidget(interior);
  raiz->addWidget(area);

  connect(estado_, &Estado::cambio, this, &PaginaInicio::refrescar);
  refrescar();
}

void PaginaInicio::construirCabecera(QVBoxLayout* raiz) {
  auto* marco = new QFrame;
  marco->setObjectName(QStringLiteral("cabeceraPatrimonio"));
  marco->setStyleSheet(
      QStringLiteral("#cabeceraPatrimonio { background-color: %1; border: 1px "
                     "solid %2; border-radius: 8px; }")
          .arg(QString::fromLatin1(paleta::kPapel),
               QString::fromLatin1(paleta::kLinea)));

  auto* izquierda = new QVBoxLayout;
  izquierda->setSpacing(2);
  izquierda->addWidget(etiqueta(tr("EN LA CUENTA"), 8, paleta::kTintaSuave));
  cifraCuenta_ = etiqueta(QStringLiteral("$0.00"), 22, paleta::kTintaSuave, true);
  izquierda->addWidget(cifraCuenta_);
  izquierda->addWidget(
      etiqueta(tr("Lo que ves en el banco"), 8, paleta::kTintaSuave));

  auto* separador = new QFrame;
  separador->setFrameShape(QFrame::VLine);
  separador->setStyleSheet(
      QStringLiteral("color: %1;").arg(QString::fromLatin1(paleta::kLinea)));

  auto* derecha = new QVBoxLayout;
  derecha->setSpacing(2);
  derecha->addWidget(etiqueta(tr("PATRIMONIO REAL"), 8, paleta::kTinta, true));
  cifraPatrimonio_ = etiqueta(QStringLiteral("$0.00"), 22, paleta::kTinta, true);
  derecha->addWidget(cifraPatrimonio_);
  derecha->addWidget(
      etiqueta(tr("Lo que de verdad es tuyo"), 8, paleta::kTintaSuave));

  auto* cifras = new QHBoxLayout;
  cifras->addLayout(izquierda, 1);
  cifras->addWidget(separador);
  cifras->addSpacing(16);
  cifras->addLayout(derecha, 1);

  explicacion_ = etiqueta(QString(), 9, paleta::kTintaSuave);
  explicacion_->setWordWrap(true);

  auto* columna = new QVBoxLayout(marco);
  columna->setContentsMargins(20, 16, 20, 16);
  columna->addLayout(cifras);
  columna->addSpacing(10);
  columna->addWidget(explicacion_);

  raiz->addWidget(marco);
}

void PaginaInicio::refrescar() {
  const ResumenPatrimonio& r = estado_->resumen();
  cifraCuenta_->setText(pesos(r.enLaCuenta));
  cifraPatrimonio_->setText(pesos(r.patrimonioReal));
  cifraPatrimonio_->setStyleSheet(
      QStringLiteral("color: %1;")
          .arg(QString::fromLatin1(r.patrimonioReal < 0 ? paleta::kNegativo
                                                        : paleta::kTinta)));
  explicacion_->setText(QString::fromStdString(explicarDiferencia(r)));

  const bool vacia = estado_->sobres().empty();
  panelVacio_->setVisible(vacia);
  panelConSobres_->setVisible(!vacia);

  // Se rehacen las tarjetas: son pocas y así no hay que sincronizar nada.
  while (QLayoutItem* item = rejillaSobres_->takeAt(0)) {
    if (QWidget* w = item->widget()) w->deleteLater();
    delete item;
  }

  int fila = 0;
  int columna = 0;
  for (const Sobre& s : estado_->sobres()) {
    auto* tarjeta = new TarjetaSobre(s, estado_->saldoDe(s.id));
    connect(tarjeta, &TarjetaSobre::editar, this,
            &PaginaInicio::pidenEditarSobre);
    connect(tarjeta, &TarjetaSobre::verMovimientos, this,
            &PaginaInicio::pidenVerMovimientosDe);
    connect(tarjeta, &TarjetaSobre::archivar, this, [this](Id id) {
      QString error;
      if (estado_->repositorio().archivarSobre(id, true, &error)) {
        estado_->recargar();
      }
    });
    connect(tarjeta, &TarjetaSobre::mover, this, [this](Id id, int paso) {
      QString error;
      if (estado_->repositorio().moverSobre(id, paso, &error)) {
        estado_->recargar();
      }
    });
    rejillaSobres_->addWidget(tarjeta, fila, columna);
    if (++columna == 2) {
      columna = 0;
      ++fila;
    }
  }
}

}  // namespace sobres
