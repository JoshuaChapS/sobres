#include "dialogos.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

#include "../nucleo/meta_proyeccion.h"
#include "../nucleo/reparto.h"
#include "../nucleo/validacion.h"

namespace sobres {
namespace {

QDialogButtonBox* botonesEstandar(QDialog* dialogo, const QString& textoAceptar) {
  auto* botones = new QDialogButtonBox(QDialogButtonBox::Ok |
                                           QDialogButtonBox::Cancel,
                                       dialogo);
  botones->button(QDialogButtonBox::Ok)->setText(textoAceptar);
  botones->button(QDialogButtonBox::Cancel)->setText(
      QObject::tr("Cancelar"));
  QObject::connect(botones, &QDialogButtonBox::rejected, dialogo,
                   &QDialog::reject);
  return botones;
}

void avisar(QWidget* padre, const QString& mensaje) {
  QMessageBox::warning(padre, QObject::tr("Revisa este dato"), mensaje);
}

QLabel* etiquetaSuave(const QString& texto) {
  auto* etiqueta = new QLabel(texto);
  etiqueta->setWordWrap(true);
  etiqueta->setStyleSheet(
      QStringLiteral("color: %1;").arg(QString::fromLatin1(paleta::kTintaSuave)));
  return etiqueta;
}

QDateEdit* nuevoCampoFecha(const Fecha& valor) {
  auto* campo = new QDateEdit(aQDate(valor));
  campo->setCalendarPopup(true);
  campo->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
  return campo;
}

void llenarPeriodicidad(QComboBox* combo) {
  combo->addItem(QObject::tr("Semanal"),
                 static_cast<int>(Periodicidad::Semanal));
  combo->addItem(QObject::tr("Quincenal"),
                 static_cast<int>(Periodicidad::Quincenal));
  combo->addItem(QObject::tr("Mensual"),
                 static_cast<int>(Periodicidad::Mensual));
}

Periodicidad periodicidadDe(const QComboBox* combo) {
  return static_cast<Periodicidad>(combo->currentData().toInt());
}

void seleccionarPeriodicidad(QComboBox* combo, Periodicidad p) {
  combo->setCurrentIndex(combo->findData(static_cast<int>(p)));
}

}  // namespace

// ---------------------------------------------------------------------------
// BotonColor
// ---------------------------------------------------------------------------

BotonColor::BotonColor(QWidget* padre) : QPushButton(padre) {
  setFixedWidth(120);
  actualizarApariencia();
  connect(this, &QPushButton::clicked, this, [this]() {
    const QColor elegido =
        QColorDialog::getColor(QColor(color_), this, tr("Color del sobre"));
    if (elegido.isValid()) fijarColor(elegido.name());
  });
}

void BotonColor::fijarColor(const QString& color) {
  color_ = color.isEmpty() ? QStringLiteral("#4b6bfb") : color;
  actualizarApariencia();
}

void BotonColor::actualizarApariencia() {
  setText(color_);
  const QColor c(color_);
  // Texto claro sobre fondos oscuros y al revés, para que se lea el código.
  const bool oscuro = c.lightness() < 140;
  setStyleSheet(QStringLiteral(
                    "background-color: %1; color: %2; border: 1px solid %3; "
                    "border-radius: 4px; padding: 6px;")
                    .arg(color_, oscuro ? QStringLiteral("#ffffff")
                                        : QStringLiteral("#1f2933"),
                         QString::fromLatin1(paleta::kLinea)));
}

// ---------------------------------------------------------------------------
// DialogoSobre
// ---------------------------------------------------------------------------

DialogoSobre::DialogoSobre(const Sobre& inicial, std::vector<Sobre> existentes,
                           QWidget* padre)
    : QDialog(padre), sobre_(inicial), existentes_(std::move(existentes)) {
  setWindowTitle(sobre_.id == kSinId ? tr("Nuevo sobre") : tr("Editar sobre"));
  setMinimumWidth(420);

  nombre_ = new QLineEdit(QString::fromStdString(sobre_.nombre));
  nombre_->setPlaceholderText(tr("Gasto diario, Renta, Colegiatura…"));

  tipo_ = new QComboBox;
  tipo_->addItem(tr("Propio — es tu dinero"),
                 static_cast<int>(TipoSobre::Propio));
  tipo_->addItem(tr("De terceros — solo está de paso"),
                 static_cast<int>(TipoSobre::Ajeno));
  tipo_->addItem(tr("Meta — tuyo, pero apartado"),
                 static_cast<int>(TipoSobre::Meta));
  tipo_->setCurrentIndex(tipo_->findData(static_cast<int>(sobre_.tipo)));

  color_ = new BotonColor;
  color_->fijarColor(QString::fromStdString(sobre_.color));

  auto* forma = new QFormLayout;
  forma->addRow(tr("Nombre"), nombre_);
  forma->addRow(tr("Tipo"), tipo_);
  forma->addRow(tr("Color"), color_);

  auto* raiz = new QVBoxLayout(this);
  raiz->addLayout(forma);
  raiz->addWidget(etiquetaSuave(
      tr("El dinero de un sobre de terceros aparece en el saldo de la cuenta "
         "pero no cuenta como patrimonio tuyo.")));
  auto* botones = botonesEstandar(this, tr("Guardar"));
  connect(botones, &QDialogButtonBox::accepted, this, &DialogoSobre::aceptar);
  raiz->addWidget(botones);
}

void DialogoSobre::aceptar() {
  sobre_.nombre = nombre_->text().trimmed().toStdString();
  sobre_.tipo = static_cast<TipoSobre>(tipo_->currentData().toInt());
  sobre_.color = color_->color().toStdString();

  const std::string problema = validarSobre(sobre_, existentes_);
  if (!problema.empty()) {
    avisar(this, QString::fromStdString(problema));
    return;
  }
  accept();
}

// ---------------------------------------------------------------------------
// DialogoCategoria
// ---------------------------------------------------------------------------

DialogoCategoria::DialogoCategoria(const Categoria& inicial,
                                   std::vector<Categoria> existentes,
                                   QWidget* padre)
    : QDialog(padre), categoria_(inicial), existentes_(std::move(existentes)) {
  setWindowTitle(categoria_.id == kSinId ? tr("Nueva categoría")
                                         : tr("Editar categoría"));
  setMinimumWidth(420);

  nombre_ = new QLineEdit(QString::fromStdString(categoria_.nombre));
  nombre_->setPlaceholderText(tr("Comida, Transporte, Suscripciones…"));
  icono_ = new QLineEdit(QString::fromStdString(categoria_.icono));
  icono_->setPlaceholderText(tr("Opcional: un emoji o una letra"));
  icono_->setMaxLength(4);
  color_ = new BotonColor;
  color_->fijarColor(QString::fromStdString(categoria_.color));

  auto* forma = new QFormLayout;
  forma->addRow(tr("Nombre"), nombre_);
  forma->addRow(tr("Ícono"), icono_);
  forma->addRow(tr("Color"), color_);

  auto* raiz = new QVBoxLayout(this);
  raiz->addLayout(forma);
  raiz->addWidget(etiquetaSuave(
      tr("Las categorías son independientes de los sobres: un mismo gasto "
         "sale de un sobre y se clasifica en una categoría.")));
  auto* botones = botonesEstandar(this, tr("Guardar"));
  connect(botones, &QDialogButtonBox::accepted, this,
          &DialogoCategoria::aceptar);
  raiz->addWidget(botones);
}

void DialogoCategoria::aceptar() {
  categoria_.nombre = nombre_->text().trimmed().toStdString();
  categoria_.icono = icono_->text().trimmed().toStdString();
  categoria_.color = color_->color().toStdString();

  const std::string problema = validarCategoria(categoria_, existentes_);
  if (!problema.empty()) {
    avisar(this, QString::fromStdString(problema));
    return;
  }
  accept();
}

// ---------------------------------------------------------------------------
// DialogoMovimiento
// ---------------------------------------------------------------------------

DialogoMovimiento::DialogoMovimiento(Estado* estado, const Movimiento& inicial,
                                     QWidget* padre)
    : QDialog(padre), estado_(estado), movimiento_(inicial) {
  setWindowTitle(movimiento_.id == kSinId ? tr("Nuevo movimiento")
                                          : tr("Editar movimiento"));
  setMinimumWidth(460);

  tipo_ = new QComboBox;
  tipo_->addItem(tr("Entrada — llega dinero"),
                 static_cast<int>(TipoMovimiento::Entrada));
  tipo_->addItem(tr("Gasto — sale dinero"),
                 static_cast<int>(TipoMovimiento::Gasto));
  tipo_->addItem(tr("Traspaso — de un sobre a otro"),
                 static_cast<int>(TipoMovimiento::Traspaso));
  tipo_->setCurrentIndex(tipo_->findData(static_cast<int>(movimiento_.tipo)));

  fecha_ = nuevoCampoFecha(esFechaValida(movimiento_.fecha) ? movimiento_.fecha
                                                            : Estado::hoy());

  sobreOrigen_ = new QComboBox;
  sobreDestino_ = new QComboBox;
  categoria_ = new CampoCategoria;
  llenarConSobres(sobreOrigen_, estado_->sobres());
  llenarConSobres(sobreDestino_, estado_->sobres());
  categoria_->fijarCategorias(estado_->categorias());
  seleccionarId(sobreOrigen_, movimiento_.sobreOrigen);
  seleccionarId(sobreDestino_, movimiento_.sobreDestino);
  categoria_->fijarSeleccion(movimiento_.categoria,
                             estado_->categoriasConArchivadas());

  monto_ = new CampoMonto;
  if (movimiento_.monto != 0) monto_->fijarMonto(movimiento_.monto);
  nota_ = new QLineEdit(QString::fromStdString(movimiento_.nota));
  nota_->setPlaceholderText(tr("Opcional"));

  etiquetaOrigen_ = new QLabel(tr("Sobre"));
  etiquetaDestino_ = new QLabel(tr("Sobre de destino"));
  etiquetaCategoria_ = new QLabel(tr("Categoría"));

  auto* forma = new QFormLayout;
  forma->addRow(tr("Tipo"), tipo_);
  forma->addRow(tr("Fecha"), fecha_);
  forma->addRow(etiquetaOrigen_, sobreOrigen_);
  forma->addRow(etiquetaDestino_, sobreDestino_);
  forma->addRow(etiquetaCategoria_, categoria_);
  forma->addRow(tr("Monto"), monto_);
  forma->addRow(tr("Nota"), nota_);

  auto* raiz = new QVBoxLayout(this);
  raiz->addLayout(forma);
  auto* botones = botonesEstandar(this, tr("Guardar"));
  connect(botones, &QDialogButtonBox::accepted, this,
          &DialogoMovimiento::aceptar);
  raiz->addWidget(botones);

  connect(tipo_, &QComboBox::currentIndexChanged, this,
          &DialogoMovimiento::ajustarSegunTipo);
  ajustarSegunTipo();
}

void DialogoMovimiento::ajustarSegunTipo() {
  const auto tipo = static_cast<TipoMovimiento>(tipo_->currentData().toInt());
  const bool esTraspaso = tipo == TipoMovimiento::Traspaso;
  const bool esEntrada = tipo == TipoMovimiento::Entrada;

  sobreDestino_->setVisible(esTraspaso);
  etiquetaDestino_->setVisible(esTraspaso);
  // Un traspaso no gasta dinero, solo lo mueve, así que no se categoriza.
  categoria_->setVisible(!esTraspaso);
  etiquetaCategoria_->setVisible(!esTraspaso);

  etiquetaOrigen_->setText(esEntrada    ? tr("Sobre que recibe")
                           : esTraspaso ? tr("Sobre de origen")
                                        : tr("Sobre del que sale"));
  adjustSize();
}

void DialogoMovimiento::aceptar() {
  const auto monto = monto_->monto();
  if (!monto.has_value()) {
    avisar(this, tr("El monto no se entiende. Escríbelo como 1234.56."));
    return;
  }
  movimiento_.tipo = static_cast<TipoMovimiento>(tipo_->currentData().toInt());
  movimiento_.fecha = deQDate(fecha_->date());
  movimiento_.sobreOrigen = idSeleccionado(sobreOrigen_);
  movimiento_.sobreDestino = movimiento_.tipo == TipoMovimiento::Traspaso
                                 ? idSeleccionado(sobreDestino_)
                                 : kSinId;

  if (movimiento_.tipo == TipoMovimiento::Traspaso) {
    movimiento_.categoria = kSinId;
  } else {
    if (categoria_->nombreCapturado().isEmpty()) {
      avisar(this, tr("Escribe una categoría. Si todavía no existe, se crea "
                      "sola al guardar."));
      return;
    }
    QString problemaCategoria;
    movimiento_.categoria = estado_->asegurarCategoria(
        categoria_->nombreCapturado(), &problemaCategoria);
    if (movimiento_.categoria == kSinId) {
      avisar(this, tr("No se pudo crear la categoría: %1")
                       .arg(problemaCategoria));
      return;
    }
  }
  movimiento_.monto = monto.value();
  movimiento_.nota = nota_->text().trimmed().toStdString();

  const std::string problema = validarMovimiento(movimiento_, estado_->sobres());
  if (!problema.empty()) {
    avisar(this, QString::fromStdString(problema));
    return;
  }
  accept();
}

// ---------------------------------------------------------------------------
// DialogoPasivo
// ---------------------------------------------------------------------------

DialogoPasivo::DialogoPasivo(const Pasivo& inicial, QWidget* padre)
    : QDialog(padre), pasivo_(inicial) {
  setWindowTitle(pasivo_.id == kSinId ? tr("Nuevo pasivo") : tr("Editar pasivo"));
  setMinimumWidth(420);

  nombre_ = new QLineEdit(QString::fromStdString(pasivo_.nombre));
  nombre_->setPlaceholderText(tr("Tarjeta de crédito, préstamo…"));
  saldo_ = new CampoMonto;
  saldo_->fijarMonto(pasivo_.saldo);
  nota_ = new QLineEdit(QString::fromStdString(pasivo_.nota));
  nota_->setPlaceholderText(tr("Opcional: fecha de corte, banco…"));

  auto* forma = new QFormLayout;
  forma->addRow(tr("Nombre"), nombre_);
  forma->addRow(tr("Saldo que debes"), saldo_);
  forma->addRow(tr("Nota"), nota_);

  auto* raiz = new QVBoxLayout(this);
  raiz->addLayout(forma);
  raiz->addWidget(etiquetaSuave(
      tr("Este saldo se resta del patrimonio real. Actualízalo cuando cambie "
         "el estado de cuenta.")));
  auto* botones = botonesEstandar(this, tr("Guardar"));
  connect(botones, &QDialogButtonBox::accepted, this, &DialogoPasivo::aceptar);
  raiz->addWidget(botones);
}

void DialogoPasivo::aceptar() {
  const auto saldo = saldo_->monto();
  if (!saldo.has_value()) {
    avisar(this, tr("El saldo no se entiende. Escríbelo como 1234.56."));
    return;
  }
  pasivo_.nombre = nombre_->text().trimmed().toStdString();
  pasivo_.saldo = saldo.value();
  pasivo_.nota = nota_->text().trimmed().toStdString();

  const std::string problema = validarPasivo(pasivo_);
  if (!problema.empty()) {
    avisar(this, QString::fromStdString(problema));
    return;
  }
  accept();
}

// ---------------------------------------------------------------------------
// DialogoMeta
// ---------------------------------------------------------------------------

DialogoMeta::DialogoMeta(Estado* estado, const Meta& inicial, QWidget* padre)
    : QDialog(padre), estado_(estado), meta_(inicial) {
  setWindowTitle(meta_.id == kSinId ? tr("Nueva meta") : tr("Editar meta"));
  setMinimumWidth(480);

  nombre_ = new QLineEdit(QString::fromStdString(meta_.nombre));
  nombre_->setPlaceholderText(tr("Viaje, enganche, fondo de emergencia…"));
  objetivo_ = new CampoMonto;
  if (meta_.montoObjetivo != 0) objetivo_->fijarMonto(meta_.montoObjetivo);

  fechaObjetivo_ = nuevoCampoFecha(
      esFechaValida(meta_.fechaObjetivo)
          ? meta_.fechaObjetivo
          : sumarMeses(Estado::hoy(), 12));

  sobre_ = new QComboBox;
  llenarConSobres(sobre_, estado_->sobres());
  seleccionarId(sobre_, meta_.sobreAsociado);

  aporte_ = new CampoMonto;
  if (meta_.aportePlaneado != 0) aporte_->fijarMonto(meta_.aportePlaneado);

  periodicidad_ = new QComboBox;
  llenarPeriodicidad(periodicidad_);
  seleccionarPeriodicidad(periodicidad_, meta_.periodicidad);

  tasa_ = new QDoubleSpinBox;
  tasa_->setRange(-99.0, 100.0);
  tasa_->setDecimals(2);
  tasa_->setSuffix(QStringLiteral(" %"));
  tasa_->setValue(meta_.tasaAnual * 100.0);

  nota_ = new QLineEdit(QString::fromStdString(meta_.nota));
  nota_->setPlaceholderText(tr("Opcional"));

  vistaPrevia_ = etiquetaSuave(QString());

  auto* forma = new QFormLayout;
  forma->addRow(tr("Nombre"), nombre_);
  forma->addRow(tr("Monto objetivo"), objetivo_);
  forma->addRow(tr("Fecha objetivo"), fechaObjetivo_);
  forma->addRow(tr("Sobre asociado"), sobre_);
  forma->addRow(tr("Aporte planeado"), aporte_);
  forma->addRow(tr("Cada"), periodicidad_);
  forma->addRow(tr("Tasa anual esperada"), tasa_);
  forma->addRow(tr("Nota"), nota_);

  auto* raiz = new QVBoxLayout(this);
  raiz->addLayout(forma);
  raiz->addWidget(vistaPrevia_);
  auto* botones = botonesEstandar(this, tr("Guardar"));
  connect(botones, &QDialogButtonBox::accepted, this, &DialogoMeta::aceptar);
  raiz->addWidget(botones);

  connect(objetivo_, &QLineEdit::textChanged, this,
          &DialogoMeta::actualizarVistaPrevia);
  connect(aporte_, &QLineEdit::textChanged, this,
          &DialogoMeta::actualizarVistaPrevia);
  connect(fechaObjetivo_, &QDateEdit::dateChanged, this,
          &DialogoMeta::actualizarVistaPrevia);
  connect(sobre_, &QComboBox::currentIndexChanged, this,
          &DialogoMeta::actualizarVistaPrevia);
  connect(periodicidad_, &QComboBox::currentIndexChanged, this,
          &DialogoMeta::actualizarVistaPrevia);
  connect(tasa_, &QDoubleSpinBox::valueChanged, this,
          &DialogoMeta::actualizarVistaPrevia);
  actualizarVistaPrevia();
}

void DialogoMeta::actualizarVistaPrevia() {
  Meta borrador = meta_;
  borrador.montoObjetivo = objetivo_->monto().value_or(0);
  borrador.fechaObjetivo = deQDate(fechaObjetivo_->date());
  borrador.sobreAsociado = idSeleccionado(sobre_);
  borrador.aportePlaneado = aporte_->monto().value_or(0);
  borrador.periodicidad = periodicidadDe(periodicidad_);
  borrador.tasaAnual = tasa_->value() / 100.0;

  if (borrador.montoObjetivo <= 0 || borrador.sobreAsociado == kSinId) {
    vistaPrevia_->setText(
        tr("Captura el monto objetivo y el sobre para ver la proyección."));
    return;
  }
  const Centavos saldo = estado_->saldoDe(borrador.sobreAsociado);
  const ProyeccionMeta p = proyectarMeta(borrador, saldo, Estado::hoy());
  QString texto = tr("Con %1 en el sobre: ")
                      .arg(pesos(saldo)) +
                  QString::fromStdString(
                      resumirProyeccion(p, borrador.periodicidad));
  if (p.proyectable && p.aporteRequerido > 0) {
    texto += tr("  Aporte necesario: %1.").arg(pesos(p.aporteRequerido));
  }
  vistaPrevia_->setText(texto);
}

void DialogoMeta::aceptar() {
  const auto objetivo = objetivo_->monto();
  if (!objetivo.has_value()) {
    avisar(this, tr("El monto objetivo no se entiende."));
    return;
  }
  const auto aporte = aporte_->estaVacio() ? std::optional<Centavos>(0)
                                           : aporte_->monto();
  if (!aporte.has_value()) {
    avisar(this, tr("El aporte planeado no se entiende."));
    return;
  }
  meta_.nombre = nombre_->text().trimmed().toStdString();
  meta_.montoObjetivo = objetivo.value();
  meta_.fechaObjetivo = deQDate(fechaObjetivo_->date());
  meta_.sobreAsociado = idSeleccionado(sobre_);
  meta_.aportePlaneado = aporte.value();
  meta_.periodicidad = periodicidadDe(periodicidad_);
  meta_.tasaAnual = tasa_->value() / 100.0;
  meta_.nota = nota_->text().trimmed().toStdString();

  const std::string problema = validarMeta(meta_, estado_->sobres());
  if (!problema.empty()) {
    avisar(this, QString::fromStdString(problema));
    return;
  }
  accept();
}

// ---------------------------------------------------------------------------
// DialogoRecurrente
// ---------------------------------------------------------------------------

DialogoRecurrente::DialogoRecurrente(Estado* estado, const Recurrente& inicial,
                                     QWidget* padre)
    : QDialog(padre), estado_(estado), recurrente_(inicial) {
  setWindowTitle(recurrente_.id == kSinId ? tr("Nuevo recurrente")
                                          : tr("Editar recurrente"));
  setMinimumWidth(460);

  nombre_ = new QLineEdit(QString::fromStdString(recurrente_.nombre));
  nombre_->setPlaceholderText(tr("Renta, gimnasio, sueldo…"));

  tipo_ = new QComboBox;
  tipo_->addItem(tr("Entrada"), static_cast<int>(TipoMovimiento::Entrada));
  tipo_->addItem(tr("Gasto"), static_cast<int>(TipoMovimiento::Gasto));
  tipo_->addItem(tr("Traspaso"), static_cast<int>(TipoMovimiento::Traspaso));
  tipo_->setCurrentIndex(tipo_->findData(static_cast<int>(recurrente_.tipo)));

  sobreOrigen_ = new QComboBox;
  sobreDestino_ = new QComboBox;
  categoria_ = new CampoCategoria;
  llenarConSobres(sobreOrigen_, estado_->sobres());
  llenarConSobres(sobreDestino_, estado_->sobres());
  categoria_->fijarCategorias(estado_->categorias());
  seleccionarId(sobreOrigen_, recurrente_.sobreOrigen);
  seleccionarId(sobreDestino_, recurrente_.sobreDestino);
  categoria_->fijarSeleccion(recurrente_.categoria,
                             estado_->categoriasConArchivadas());

  monto_ = new CampoMonto;
  if (recurrente_.monto != 0) monto_->fijarMonto(recurrente_.monto);

  periodicidad_ = new QComboBox;
  llenarPeriodicidad(periodicidad_);
  seleccionarPeriodicidad(periodicidad_, recurrente_.periodicidad);

  proximaFecha_ = nuevoCampoFecha(esFechaValida(recurrente_.proximaFecha)
                                      ? recurrente_.proximaFecha
                                      : Estado::hoy());

  activo_ = new QCheckBox(tr("Activo"));
  activo_->setChecked(recurrente_.activo);
  nota_ = new QLineEdit(QString::fromStdString(recurrente_.nota));
  nota_->setPlaceholderText(tr("Opcional"));

  etiquetaDestino_ = new QLabel(tr("Sobre de destino"));

  auto* forma = new QFormLayout;
  forma->addRow(tr("Nombre"), nombre_);
  forma->addRow(tr("Tipo"), tipo_);
  forma->addRow(tr("Sobre"), sobreOrigen_);
  forma->addRow(etiquetaDestino_, sobreDestino_);
  forma->addRow(tr("Categoría"), categoria_);
  forma->addRow(tr("Monto"), monto_);
  forma->addRow(tr("Periodicidad"), periodicidad_);
  forma->addRow(tr("Próxima fecha"), proximaFecha_);
  forma->addRow(QString(), activo_);
  forma->addRow(tr("Nota"), nota_);

  auto* raiz = new QVBoxLayout(this);
  raiz->addLayout(forma);
  raiz->addWidget(etiquetaSuave(
      tr("Un recurrente no se registra solo: aparece en la lista cuando le "
         "toca y tú decides cuándo aplicarlo.")));
  auto* botones = botonesEstandar(this, tr("Guardar"));
  connect(botones, &QDialogButtonBox::accepted, this,
          &DialogoRecurrente::aceptar);
  raiz->addWidget(botones);

  connect(tipo_, &QComboBox::currentIndexChanged, this,
          &DialogoRecurrente::ajustarSegunTipo);
  ajustarSegunTipo();
}

void DialogoRecurrente::ajustarSegunTipo() {
  const bool esTraspaso =
      static_cast<TipoMovimiento>(tipo_->currentData().toInt()) ==
      TipoMovimiento::Traspaso;
  sobreDestino_->setVisible(esTraspaso);
  etiquetaDestino_->setVisible(esTraspaso);
  adjustSize();
}

void DialogoRecurrente::aceptar() {
  const auto monto = monto_->monto();
  if (!monto.has_value()) {
    avisar(this, tr("El monto no se entiende. Escríbelo como 1234.56."));
    return;
  }
  recurrente_.nombre = nombre_->text().trimmed().toStdString();
  recurrente_.tipo = static_cast<TipoMovimiento>(tipo_->currentData().toInt());
  recurrente_.sobreOrigen = idSeleccionado(sobreOrigen_);
  recurrente_.sobreDestino = recurrente_.tipo == TipoMovimiento::Traspaso
                                 ? idSeleccionado(sobreDestino_)
                                 : kSinId;

  if (recurrente_.tipo == TipoMovimiento::Traspaso) {
    recurrente_.categoria = kSinId;
  } else {
    if (categoria_->nombreCapturado().isEmpty()) {
      avisar(this, tr("Escribe una categoría. Si todavía no existe, se crea "
                      "sola al guardar."));
      return;
    }
    QString problemaCategoria;
    recurrente_.categoria = estado_->asegurarCategoria(
        categoria_->nombreCapturado(), &problemaCategoria);
    if (recurrente_.categoria == kSinId) {
      avisar(this, tr("No se pudo crear la categoría: %1")
                       .arg(problemaCategoria));
      return;
    }
  }
  recurrente_.monto = monto.value();
  recurrente_.periodicidad = periodicidadDe(periodicidad_);
  recurrente_.proximaFecha = deQDate(proximaFecha_->date());
  recurrente_.activo = activo_->isChecked();
  recurrente_.nota = nota_->text().trimmed().toStdString();

  const std::string problema =
      validarRecurrente(recurrente_, estado_->sobres());
  if (!problema.empty()) {
    avisar(this, QString::fromStdString(problema));
    return;
  }
  accept();
}

// ---------------------------------------------------------------------------
// DialogoPlantilla
// ---------------------------------------------------------------------------

DialogoPlantilla::DialogoPlantilla(Estado* estado,
                                   const PlantillaReparto& inicial,
                                   QWidget* padre)
    : QDialog(padre), estado_(estado), plantilla_(inicial) {
  setWindowTitle(plantilla_.id == kSinId ? tr("Nueva plantilla de reparto")
                                         : tr("Editar plantilla de reparto"));
  setMinimumWidth(620);

  nombre_ = new QLineEdit(QString::fromStdString(plantilla_.nombre));
  nombre_->setPlaceholderText(tr("Sueldo quincenal, aguinaldo…"));

  modo_ = new QComboBox;
  modo_->addItem(tr("Porcentajes de un monto"),
                 static_cast<int>(ModoReparto::Porcentajes));
  modo_->addItem(tr("Montos fijos"),
                 static_cast<int>(ModoReparto::MontosFijos));
  modo_->setCurrentIndex(modo_->findData(static_cast<int>(plantilla_.modo)));

  montoSugerido_ = new CampoMonto;
  if (plantilla_.montoSugerido != 0) {
    montoSugerido_->fijarMonto(plantilla_.montoSugerido);
  }

  sobreFuente_ = new QComboBox;
  llenarConSobres(sobreFuente_, estado_->sobres(), true,
                  tr("Ninguno — el dinero llega de fuera"));
  seleccionarId(sobreFuente_, plantilla_.sobreFuente);

  categoria_ = new CampoCategoria;
  categoria_->fijarCategorias(estado_->categorias());
  categoria_->fijarSeleccion(plantilla_.categoria,
                             estado_->categoriasConArchivadas());

  tabla_ = new QTableWidget(0, 3);
  tabla_->setHorizontalHeaderLabels(
      {tr("Sobre"), tr("Porcentaje o monto"), tr("Recibe el resto")});
  tabla_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  tabla_->horizontalHeader()->setSectionResizeMode(
      1, QHeaderView::ResizeToContents);
  tabla_->horizontalHeader()->setSectionResizeMode(
      2, QHeaderView::ResizeToContents);
  tabla_->verticalHeader()->setVisible(false);
  tabla_->setSelectionBehavior(QAbstractItemView::SelectRows);
  for (const LineaReparto& l : plantilla_.lineas) {
    tabla_->insertRow(tabla_->rowCount());
    ponerLinea(tabla_->rowCount() - 1, l);
  }
  ajustarAlturaDeTabla(tabla_, 8, 3);

  suma_ = etiquetaSuave(QString());

  auto* agregar = new QPushButton(tr("Agregar línea"));
  auto* quitar = new QPushButton(tr("Quitar la seleccionada"));
  connect(agregar, &QPushButton::clicked, this,
          &DialogoPlantilla::agregarLinea);
  connect(quitar, &QPushButton::clicked, this, &DialogoPlantilla::quitarLinea);

  auto* forma = new QFormLayout;
  forma->addRow(tr("Nombre"), nombre_);
  forma->addRow(tr("Modo"), modo_);
  forma->addRow(tr("Monto que se suele repartir"), montoSugerido_);
  forma->addRow(tr("Sale de"), sobreFuente_);
  forma->addRow(tr("Categoría"), categoria_);

  auto* fila = new QHBoxLayout;
  fila->addWidget(agregar);
  fila->addWidget(quitar);
  fila->addStretch();

  auto* raiz = new QVBoxLayout(this);
  raiz->addLayout(forma);
  raiz->addWidget(tabla_);
  raiz->addLayout(fila);
  raiz->addWidget(suma_);
  auto* botones = botonesEstandar(this, tr("Guardar"));
  connect(botones, &QDialogButtonBox::accepted, this,
          &DialogoPlantilla::aceptar);
  raiz->addWidget(botones);

  connect(modo_, &QComboBox::currentIndexChanged, this,
          &DialogoPlantilla::actualizarSuma);
  connect(tabla_, &QTableWidget::cellChanged, this,
          &DialogoPlantilla::actualizarSuma);
  actualizarSuma();
}

void DialogoPlantilla::ponerLinea(int fila, const LineaReparto& linea) {
  auto* combo = new QComboBox;
  llenarConSobres(combo, estado_->sobres());
  seleccionarId(combo, linea.sobre);
  tabla_->setCellWidget(fila, 0, combo);

  const bool porcentajes =
      static_cast<ModoReparto>(modo_->currentData().toInt()) ==
      ModoReparto::Porcentajes;
  const QString texto =
      porcentajes
          ? QString::fromStdString(formatearPuntosBase(linea.valor))
                .remove(QLatin1Char('%'))
          : QString::fromStdString(
                formatearPesos(static_cast<Centavos>(linea.valor), false));
  tabla_->setItem(fila, 1, new QTableWidgetItem(texto));

  auto* resto = new QCheckBox;
  resto->setChecked(linea.recibeResto);
  auto* contenedor = new QWidget;
  auto* caja = new QHBoxLayout(contenedor);
  caja->setContentsMargins(0, 0, 0, 0);
  caja->addWidget(resto, 0, Qt::AlignCenter);
  tabla_->setCellWidget(fila, 2, contenedor);
}

void DialogoPlantilla::agregarLinea() {
  if (estado_->sobres().empty()) {
    avisar(this, tr("Primero crea al menos un sobre."));
    return;
  }
  LineaReparto nueva;
  nueva.sobre = estado_->sobres().front().id;
  nueva.valor = 0;
  tabla_->insertRow(tabla_->rowCount());
  ponerLinea(tabla_->rowCount() - 1, nueva);
  ajustarAlturaDeTabla(tabla_, 8, 3);
  actualizarSuma();
}

void DialogoPlantilla::quitarLinea() {
  const int fila = tabla_->currentRow();
  if (fila >= 0) {
    tabla_->removeRow(fila);
    ajustarAlturaDeTabla(tabla_, 8, 3);
    actualizarSuma();
  }
}

PlantillaReparto DialogoPlantilla::leerDeLaTabla() const {
  PlantillaReparto p = plantilla_;
  p.nombre = nombre_->text().trimmed().toStdString();
  p.modo = static_cast<ModoReparto>(modo_->currentData().toInt());
  p.montoSugerido = montoSugerido_->monto().value_or(0);
  p.sobreFuente = idSeleccionado(sobreFuente_);
  // La categoría se resuelve al aceptar, porque puede implicar crearla y esto
  // se llama también cada vez que cambia una celda.
  p.categoria = plantilla_.categoria;
  p.lineas.clear();

  for (int fila = 0; fila < tabla_->rowCount(); ++fila) {
    LineaReparto linea;
    auto* combo = qobject_cast<QComboBox*>(tabla_->cellWidget(fila, 0));
    linea.sobre = combo ? idSeleccionado(combo) : kSinId;

    const QTableWidgetItem* celda = tabla_->item(fila, 1);
    const QString texto = celda ? celda->text() : QString();
    if (p.modo == ModoReparto::Porcentajes) {
      // El porcentaje se guarda en puntos base: 25.5% son 2550.
      QString limpio = texto;
      limpio.remove(QLatin1Char('%'));
      bool ok = false;
      const double valor = limpio.trimmed().toDouble(&ok);
      linea.valor = ok ? redondearCentavos(valor * 100.0) : 0;
    } else {
      linea.valor = leerPesos(texto.toStdString()).value_or(0);
    }

    auto* contenedor = tabla_->cellWidget(fila, 2);
    auto* casilla = contenedor ? contenedor->findChild<QCheckBox*>() : nullptr;
    linea.recibeResto = casilla && casilla->isChecked();

    p.lineas.push_back(linea);
  }
  return p;
}

void DialogoPlantilla::actualizarSuma() {
  const PlantillaReparto p = leerDeLaTabla();
  if (p.modo == ModoReparto::Porcentajes) {
    const std::int64_t suma = sumaDePuntosBase(p);
    const QString texto =
        tr("Los porcentajes suman %1.")
            .arg(QString::fromStdString(formatearPuntosBase(suma)));
    suma_->setText(suma == 10000
                       ? texto
                       : texto + tr(" Tienen que sumar exactamente 100%."));
  } else {
    Centavos total = 0;
    for (const LineaReparto& l : p.lineas) total += l.valor;
    suma_->setText(tr("Los montos suman %1.").arg(pesos(total)));
  }
}

void DialogoPlantilla::aceptar() {
  plantilla_ = leerDeLaTabla();

  const QString nombreCategoria = categoria_->nombreCapturado();
  if (nombreCategoria.isEmpty()) {
    plantilla_.categoria = kSinId;
  } else {
    QString problemaCategoria;
    plantilla_.categoria =
        estado_->asegurarCategoria(nombreCategoria, &problemaCategoria);
    if (plantilla_.categoria == kSinId) {
      avisar(this,
             tr("No se pudo crear la categoría: %1").arg(problemaCategoria));
      return;
    }
  }

  const std::string problema =
      validarPlantillaReparto(plantilla_, estado_->sobres());
  if (!problema.empty()) {
    avisar(this, QString::fromStdString(problema));
    return;
  }
  accept();
}

// ---------------------------------------------------------------------------
// DialogoAplicarReparto
// ---------------------------------------------------------------------------

DialogoAplicarReparto::DialogoAplicarReparto(Estado* estado,
                                             const PlantillaReparto& plantilla,
                                             QWidget* padre)
    : QDialog(padre), estado_(estado), plantilla_(plantilla) {
  setWindowTitle(tr("Aplicar «%1»").arg(QString::fromStdString(plantilla_.nombre)));
  setMinimumWidth(520);

  fecha_ = nuevoCampoFecha(Estado::hoy());
  monto_ = new CampoMonto;
  const bool porcentajes = plantilla_.modo == ModoReparto::Porcentajes;
  if (porcentajes) {
    monto_->fijarMonto(plantilla_.montoSugerido);
  } else {
    Centavos total = 0;
    for (const LineaReparto& l : plantilla_.lineas) total += l.valor;
    monto_->fijarMonto(total);
    monto_->setEnabled(false);  // en montos fijos el total no se elige
  }

  vistaPrevia_ = new QTableWidget(0, 2);
  vistaPrevia_->setHorizontalHeaderLabels({tr("Sobre"), tr("Le toca")});
  vistaPrevia_->horizontalHeader()->setSectionResizeMode(0,
                                                         QHeaderView::Stretch);
  vistaPrevia_->verticalHeader()->setVisible(false);
  vistaPrevia_->setEditTriggers(QAbstractItemView::NoEditTriggers);

  aviso_ = etiquetaSuave(QString());

  auto* forma = new QFormLayout;
  forma->addRow(tr("Fecha"), fecha_);
  forma->addRow(tr("Monto a repartir"), monto_);

  auto* raiz = new QVBoxLayout(this);
  raiz->addLayout(forma);
  raiz->addWidget(vistaPrevia_);
  raiz->addWidget(aviso_);
  auto* botones = botonesEstandar(this, tr("Aplicar"));
  connect(botones, &QDialogButtonBox::accepted, this,
          &DialogoAplicarReparto::aceptar);
  raiz->addWidget(botones);

  connect(monto_, &QLineEdit::textChanged, this,
          &DialogoAplicarReparto::actualizarVistaPrevia);
  actualizarVistaPrevia();
}

void DialogoAplicarReparto::actualizarVistaPrevia() {
  const ResultadoReparto r =
      calcularReparto(plantilla_, monto_->monto().value_or(0));
  vistaPrevia_->setRowCount(0);
  if (!r.valido) {
    aviso_->setText(QString::fromStdString(r.problema));
    return;
  }
  for (const AsignacionReparto& a : r.asignaciones) {
    const int fila = vistaPrevia_->rowCount();
    vistaPrevia_->insertRow(fila);
    vistaPrevia_->setItem(fila, 0,
                          new QTableWidgetItem(estado_->nombreDeSobre(a.sobre)));
    auto* celda = new QTableWidgetItem(pesos(a.monto));
    celda->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    vistaPrevia_->setItem(fila, 1, celda);
  }
  ajustarAlturaDeTabla(vistaPrevia_, 8, 2);
  const QString destino =
      plantilla_.sobreFuente == kSinId
          ? tr("Se registrarán %1 entradas.").arg(r.asignaciones.size())
          : tr("Se registrarán %1 traspasos desde %2.")
                .arg(r.asignaciones.size())
                .arg(estado_->nombreDeSobre(plantilla_.sobreFuente));
  aviso_->setText(tr("Total repartido: %1. ").arg(pesos(r.total)) + destino);
}

Fecha DialogoAplicarReparto::fecha() const { return deQDate(fecha_->date()); }

Centavos DialogoAplicarReparto::montoTotal() const {
  return monto_->monto().value_or(0);
}

void DialogoAplicarReparto::aceptar() {
  const ResultadoReparto r = calcularReparto(plantilla_, montoTotal());
  if (!r.valido) {
    avisar(this, QString::fromStdString(r.problema));
    return;
  }
  accept();
}

}  // namespace sobres
