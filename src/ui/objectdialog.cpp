#include "ui/objectdialog.h"

#include <QCheckBox>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace looping;

namespace {
enum FieldRoles { TypeRole = Qt::UserRole, LengthRole, PrecisionRole, ScaleRole, CollationRole, ComplementRole, FreeTypeRole, CommentRole };
}

FieldDialog::FieldDialog(Field field, QWidget *parent) : QDialog(parent), m_field(std::move(field))
{
    setWindowTitle(tr("Rubrique"));
    resize(600, 650);
    auto *layout = new QVBoxLayout(this);
    auto *identity = new QFormLayout;
    m_name = new QLineEdit(m_field.name);
    m_logicalName = new QLineEdit(m_field.logicalName);
    identity->addRow(tr("Nom"), m_name);
    identity->addRow(tr("Nom logique"), m_logicalName);
    layout->addLayout(identity);

    auto *typeBox = new QGroupBox(tr("Type"));
    auto *typeLayout = new QGridLayout(typeBox);
    auto *textBox = new QGroupBox(tr("Texte"));
    auto *textLayout = new QGridLayout(textBox);
    m_encoding = new QButtonGroup(this);
    m_textShape = new QButtonGroup(this);
    const QStringList encodings{tr("Caractères ASCII"), tr("Caractères Unicode"), tr("Binaire")};
    for (int i = 0; i < encodings.size(); ++i) {
        auto *button = new QRadioButton(encodings[i]);
        m_encoding->addButton(button, i); textLayout->addWidget(button, i, 0);
    }
    const QStringList shapes{tr("Variable"), tr("Fixe"), tr("Volumineux")};
    for (int i = 0; i < shapes.size(); ++i) {
        auto *button = new QRadioButton(shapes[i]);
        m_textShape->addButton(button, i); textLayout->addWidget(button, i, 1);
    }
    m_length = new QSpinBox; m_length->setRange(1, 1000000); m_length->setValue(qMax(1, m_field.length));
    m_collation = new QLineEdit(m_field.collation);
    textLayout->addWidget(new QLabel(tr("Longueur")), 3, 0); textLayout->addWidget(m_length, 3, 1);
    textLayout->addWidget(new QLabel(tr("Collation")), 4, 0); textLayout->addWidget(m_collation, 4, 1);
    typeLayout->addWidget(textBox, 0, 0, 2, 1);

    m_directType = new QButtonGroup(this);
    auto addDirectGroup = [this, typeLayout](const QString &title, const QList<QPair<DataType, QString>> &types, int row, int column, int rowSpan = 1, int columnSpan = 1) {
        auto *box = new QGroupBox(title); auto *boxLayout = new QVBoxLayout(box);
        for (const auto &[type, label] : types) {
            auto *button = new QRadioButton(label); m_directType->addButton(button, static_cast<int>(type)); boxLayout->addWidget(button);
        }
        boxLayout->addStretch(); typeLayout->addWidget(box, row, column, rowSpan, columnSpan);
    };
    addDirectGroup(tr("Numérique"), {{DataType::Integer, tr("Entier")}, {DataType::Decimal, tr("Décimal")},
        {DataType::Real, tr("Réel")}, {DataType::Money, tr("Monétaire")}, {DataType::Counter, tr("Compteur")}}, 0, 1);
    addDirectGroup(tr("Date / Heure"), {{DataType::Date, tr("Date")}, {DataType::Time, tr("Heure")},
        {DataType::DateTime, tr("Date-Heure")}}, 0, 2);
    addDirectGroup(tr("Autre"), {{DataType::Boolean, tr("Booléen")}, {DataType::Xml, tr("Xml")},
        {DataType::Geometry, tr("Géométrique")}, {DataType::Geography, tr("Géographique")}, {DataType::Free, tr("Libre")}}, 1, 1, 1, 2);

    auto *details = new QFormLayout;
    m_precision = new QSpinBox; m_precision->setRange(1, 65); m_precision->setValue(qMax(1, m_field.precision));
    m_scale = new QSpinBox; m_scale->setRange(0, 30); m_scale->setValue(qMax(0, m_field.scale));
    m_freeType = new QLineEdit(m_field.freeType); m_freeType->setPlaceholderText(tr("Type SQL personnalisé"));
    details->addRow(tr("Précision"), m_precision); details->addRow(tr("Décimales"), m_scale); details->addRow(tr("Type libre"), m_freeType);
    typeLayout->addLayout(details, 2, 0, 1, 3);
    layout->addWidget(typeBox);

    auto *properties = new QGroupBox(tr("Propriétés")); auto *propertiesLayout = new QGridLayout(properties);
    m_primaryKey = new QCheckBox(tr("Identifiant")); m_primaryKey->setChecked(m_field.primaryKey);
    m_notNull = new QCheckBox(tr("NOT NULL")); m_notNull->setChecked(m_field.notNull || m_field.primaryKey);
    m_unique = new QCheckBox(tr("UNIQUE")); m_unique->setChecked(m_field.unique);
    m_complement = new QLineEdit(m_field.complement);
    propertiesLayout->addWidget(m_primaryKey, 0, 0); propertiesLayout->addWidget(m_notNull, 0, 1); propertiesLayout->addWidget(m_unique, 0, 2);
    propertiesLayout->addWidget(new QLabel(tr("Complément")), 1, 0); propertiesLayout->addWidget(m_complement, 1, 1, 1, 2);
    layout->addWidget(properties);
    auto synchronizeIdentifier = [this](bool identifier) {
        if (identifier) m_notNull->setChecked(true);
        m_notNull->setEnabled(!identifier);
    };
    connect(m_primaryKey, &QCheckBox::toggled, this, synchronizeIdentifier);
    synchronizeIdentifier(m_primaryKey->isChecked());
    auto *commentBox = new QGroupBox(tr("Commentaire")); auto *commentLayout = new QVBoxLayout(commentBox);
    m_comment = new QTextEdit(m_field.comment); m_comment->setMaximumHeight(80); commentLayout->addWidget(m_comment); layout->addWidget(commentBox);

    auto clearGroup = [](QButtonGroup *group) { group->setExclusive(false); for (auto *button : group->buttons()) button->setChecked(false); group->setExclusive(true); };
    connect(m_encoding, &QButtonGroup::idClicked, this, [this, clearGroup](int) { clearGroup(m_directType); if (m_textShape->checkedId() < 0) m_textShape->button(0)->setChecked(true); updateAvailability(); });
    connect(m_textShape, &QButtonGroup::idClicked, this, [this, clearGroup](int) { clearGroup(m_directType); if (m_encoding->checkedId() < 0) m_encoding->button(0)->setChecked(true); updateAvailability(); });
    connect(m_directType, &QButtonGroup::idClicked, this, [this, clearGroup](int) { clearGroup(m_encoding); clearGroup(m_textShape); updateAvailability(); });

    int encoding = -1, shape = -1;
    switch (m_field.type) {
    case DataType::VarChar: encoding = 0; shape = 0; break;
    case DataType::Char: encoding = 0; shape = 1; break;
    case DataType::Text: encoding = 0; shape = 2; break;
    case DataType::NVarChar: encoding = 1; shape = 0; break;
    case DataType::NChar: encoding = 1; shape = 1; break;
    case DataType::NText: encoding = 1; shape = 2; break;
    case DataType::VarBinary: encoding = 2; shape = 0; break;
    case DataType::Binary: encoding = 2; shape = 1; break;
    case DataType::Blob: encoding = 2; shape = 2; break;
    default: if (auto *button = m_directType->button(static_cast<int>(m_field.type))) button->setChecked(true); break;
    }
    if (encoding >= 0) { m_encoding->button(encoding)->setChecked(true); m_textShape->button(shape)->setChecked(true); }
    updateAvailability();
    connect(m_name, &QLineEdit::textEdited, this, [this](const QString &value) { m_logicalName->setText(normalizedLogicalName(value)); });
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void FieldDialog::updateAvailability()
{
    const bool text = m_encoding->checkedId() >= 0;
    const bool sized = text && m_textShape->checkedId() != 2;
    m_length->setEnabled(sized); m_collation->setEnabled(text && m_encoding->checkedId() != 2);
    const int direct = m_directType->checkedId();
    const bool decimal = direct == static_cast<int>(DataType::Decimal) || direct == static_cast<int>(DataType::Money);
    m_precision->setEnabled(decimal); m_scale->setEnabled(decimal);
    m_freeType->setEnabled(direct == static_cast<int>(DataType::Free));
}

Field FieldDialog::field() const
{
    Field result = m_field;
    result.name = m_name->text().trimmed(); result.logicalName = m_logicalName->text().trimmed();
    if (m_directType->checkedId() >= 0) result.type = static_cast<DataType>(m_directType->checkedId());
    else {
        static const DataType textTypes[3][3] = {{DataType::VarChar, DataType::Char, DataType::Text},
            {DataType::NVarChar, DataType::NChar, DataType::NText}, {DataType::VarBinary, DataType::Binary, DataType::Blob}};
        result.type = textTypes[qBound(0, m_encoding->checkedId(), 2)][qBound(0, m_textShape->checkedId(), 2)];
    }
    result.length = m_length->value(); result.precision = m_precision->value(); result.scale = m_scale->value();
    result.collation = m_collation->text().trimmed(); result.freeType = m_freeType->text().trimmed();
    result.primaryKey = m_primaryKey->isChecked(); result.notNull = result.primaryKey || m_notNull->isChecked(); result.unique = m_unique->isChecked();
    result.complement = m_complement->text().trimmed(); result.comment = m_comment->toPlainText();
    return result;
}

ObjectDialog::ObjectDialog(const QString &title, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(title);
    resize(650, 600);
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    auto *form = new QFormLayout;
    m_name = new QLineEdit;
    m_logicalName = new QLineEdit;
    form->addRow(tr("Nom"), m_name);
    form->addRow(tr("Nom logique"), m_logicalName);
    layout->addLayout(form);
    auto *rubriques = new QGroupBox(tr("Rubriques"));
    auto *rubriquesLayout = new QHBoxLayout(rubriques);
    m_fields = new QTableWidget(0, 6);
    m_fields->setHorizontalHeaderLabels({tr("Id"), tr("Nom conceptuel"), tr("Nom logique"), tr("Type"), tr("NOT NULL"), tr("UNIQUE")});
    m_fields->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_fields->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_fields->setSelectionBehavior(QAbstractItemView::SelectRows);
    rubriquesLayout->addWidget(m_fields, 1);
    auto *commands = new QVBoxLayout;
    auto *add = new QPushButton(tr("Ajouter"));
    auto *duplicate = new QPushButton(tr("Dupliquer")); duplicate->setEnabled(false);
    auto *modify = new QPushButton(tr("Modifier")); modify->setEnabled(false);
    auto *rename = new QPushButton(tr("Renommer")); rename->setEnabled(false);
    auto *remove = new QPushButton(tr("Supprimer"));
    auto *up = new QPushButton(QStringLiteral("↑"));
    auto *down = new QPushButton(QStringLiteral("↓"));
    remove->setEnabled(false); up->setEnabled(false); down->setEnabled(false);
    for (auto *button : {add, duplicate, modify, rename, remove, up, down}) { button->setMinimumWidth(78); commands->addWidget(button); }
    commands->addStretch(); rubriquesLayout->addLayout(commands);
    layout->addWidget(rubriques, 1);
    auto *comment = new QGroupBox(tr("Commentaire")); auto *commentLayout = new QVBoxLayout(comment);
    auto *commentEdit = new QTextEdit; commentEdit->setMaximumHeight(70); commentLayout->addWidget(commentEdit); layout->addWidget(comment);
    auto *generation = new QGroupBox(tr("Génération MLD")); auto *generationLayout = new QVBoxLayout(generation);
    generationLayout->addWidget(new QCheckBox(tr("Classe d’entités fictive (ne pas générer de table pour cette classe dans le MLD)")));
    layout->addWidget(generation);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(add, &QPushButton::clicked, this, &ObjectDialog::addField);
    connect(remove, &QPushButton::clicked, this, &ObjectDialog::removeField);
    connect(m_fields, &QTableWidget::currentCellChanged, this, [this, duplicate, modify, rename, remove, up, down](int row) {
        const bool selected = row >= 0;
        duplicate->setEnabled(selected); modify->setEnabled(selected); rename->setEnabled(selected);
        remove->setEnabled(selected); up->setEnabled(selected && row > 0); down->setEnabled(selected && row + 1 < m_fields->rowCount());
    });
    connect(rename, &QPushButton::clicked, this, [this] { if (m_fields->currentRow() >= 0) m_fields->editItem(m_fields->item(m_fields->currentRow(), 1)); });
    connect(modify, &QPushButton::clicked, this, &ObjectDialog::modifyField);
    connect(m_fields, &QTableWidget::cellDoubleClicked, this, [this](int row) { m_fields->setCurrentCell(row, 1); modifyField(); });
    connect(duplicate, &QPushButton::clicked, this, [this] {
        const int source = m_fields->currentRow(); if (source < 0) return;
        auto field = fieldAt(source); field.id = newId(); field.name += tr("_copie"); field.logicalName += tr("_copie");
        appendFieldRow(field); const int target = m_fields->rowCount() - 1;
        m_fields->setCurrentCell(target, 1);
    });
    auto swapRows = [this](int first, int second) {
        if (first < 0 || second < 0 || first >= m_fields->rowCount() || second >= m_fields->rowCount()) return;
        const auto firstField = fieldAt(first), secondField = fieldAt(second);
        setFieldAt(first, secondField); setFieldAt(second, firstField);
        m_fields->setCurrentCell(second, 1);
    };
    connect(up, &QPushButton::clicked, this, [this, swapRows] { const int row = m_fields->currentRow(); swapRows(row, row - 1); });
    connect(down, &QPushButton::clicked, this, [this, swapRows] { const int row = m_fields->currentRow(); swapRows(row, row + 1); });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_name, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_logicalName->setText(normalizedLogicalName(text));
    });
}

void ObjectDialog::setObject(const QString &name, const QString &logicalName, const QVector<Field> &fields)
{
    m_name->setText(name); m_logicalName->setText(logicalName);
    for (const auto &field : fields) appendFieldRow(field);
}

void ObjectDialog::addField()
{
    const int row = m_fields->rowCount();
    const QString name = tr("rubrique%1").arg(row + 1);
    Field field{newId(), name, normalizedLogicalName(name)};
    FieldDialog dialog(field, this);
    if (dialog.exec() != QDialog::Accepted || dialog.field().name.isEmpty()) return;
    appendFieldRow(dialog.field());
    m_fields->setCurrentCell(m_fields->rowCount() - 1, 1);
}

void ObjectDialog::modifyField()
{
    const int row = m_fields->currentRow(); if (row < 0) return;
    FieldDialog dialog(fieldAt(row), this);
    if (dialog.exec() == QDialog::Accepted && !dialog.field().name.isEmpty()) setFieldAt(row, dialog.field());
}

void ObjectDialog::appendFieldRow(const Field &field)
{
    const int row = m_fields->rowCount(); m_fields->insertRow(row);
    auto *identifier = new QCheckBox;
    auto *notNull = new QCheckBox;
    auto *unique = new QCheckBox;
    connect(identifier, &QCheckBox::toggled, this, [notNull](bool checked) {
        if (checked) notNull->setChecked(true);
        notNull->setEnabled(!checked);
    });
    m_fields->setCellWidget(row, 0, identifier); m_fields->setCellWidget(row, 4, notNull); m_fields->setCellWidget(row, 5, unique);
    m_fields->setItem(row, 1, new QTableWidgetItem); m_fields->setItem(row, 2, new QTableWidgetItem); m_fields->setItem(row, 3, new QTableWidgetItem);
    setFieldAt(row, field);
}

void ObjectDialog::setFieldAt(int row, const Field &field)
{
    auto *name = m_fields->item(row, 1); name->setText(field.name); name->setData(Qt::UserRole, field.id);
    m_fields->item(row, 2)->setText(field.logicalName);
    auto *type = m_fields->item(row, 3); type->setText(dataTypeName(field)); type->setData(TypeRole, static_cast<int>(field.type));
    type->setData(LengthRole, field.length); type->setData(PrecisionRole, field.precision); type->setData(ScaleRole, field.scale);
    type->setData(CollationRole, field.collation); type->setData(ComplementRole, field.complement);
    type->setData(FreeTypeRole, field.freeType); type->setData(CommentRole, field.comment);
    qobject_cast<QCheckBox *>(m_fields->cellWidget(row, 4))->setChecked(field.notNull || field.primaryKey);
    qobject_cast<QCheckBox *>(m_fields->cellWidget(row, 0))->setChecked(field.primaryKey);
    qobject_cast<QCheckBox *>(m_fields->cellWidget(row, 5))->setChecked(field.unique);
}

Field ObjectDialog::fieldAt(int row) const
{
    Field field;
    field.id = m_fields->item(row, 1)->data(Qt::UserRole).toString(); if (field.id.isEmpty()) field.id = newId();
    field.name = m_fields->item(row, 1)->text().trimmed(); field.logicalName = m_fields->item(row, 2)->text().trimmed();
    const auto *type = m_fields->item(row, 3); field.type = static_cast<DataType>(type->data(TypeRole).toInt());
    field.length = type->data(LengthRole).toInt(); field.precision = type->data(PrecisionRole).toInt(); field.scale = type->data(ScaleRole).toInt();
    field.collation = type->data(CollationRole).toString(); field.complement = type->data(ComplementRole).toString();
    field.freeType = type->data(FreeTypeRole).toString(); field.comment = type->data(CommentRole).toString();
    field.primaryKey = qobject_cast<QCheckBox *>(m_fields->cellWidget(row, 0))->isChecked();
    field.notNull = field.primaryKey || qobject_cast<QCheckBox *>(m_fields->cellWidget(row, 4))->isChecked(); field.unique = qobject_cast<QCheckBox *>(m_fields->cellWidget(row, 5))->isChecked();
    return field;
}

void ObjectDialog::removeField() { if (m_fields->currentRow() >= 0) m_fields->removeRow(m_fields->currentRow()); }
QString ObjectDialog::objectName() const { return m_name->text().trimmed(); }
QString ObjectDialog::logicalName() const { return m_logicalName->text().trimmed(); }

QVector<Field> ObjectDialog::fields() const
{
    QVector<Field> result;
    for (int row = 0; row < m_fields->rowCount(); ++row) {
        result.append(fieldAt(row));
    }
    return result;
}

LinkDialog::LinkDialog(Link link, QWidget *parent) : QDialog(parent), m_link(std::move(link))
{
    setWindowTitle(tr("Propriétés du lien"));
    auto *layout = new QFormLayout(this);
    m_cardinality = new QComboBox;
    m_cardinality->addItems({"0,1", "1,1", "0,n", "1,n", tr("Personnalisée")});
    const int index = qBound(0, static_cast<int>(m_link.cardinality), 4);
    m_cardinality->setCurrentIndex(index);
    auto *custom = new QHBoxLayout;
    m_customMin = new QLineEdit(m_link.customMin); m_customMin->setPlaceholderText(tr("Min")); m_customMin->setMaximumWidth(70);
    m_customMax = new QLineEdit(m_link.customMax); m_customMax->setPlaceholderText(tr("Max")); m_customMax->setMaximumWidth(70);
    custom->addWidget(new QLabel(tr("Min :"))); custom->addWidget(m_customMin); custom->addWidget(new QLabel(tr("Max :"))); custom->addWidget(m_customMax); custom->addStretch();
    m_primaryPosition = new QSpinBox; m_primaryPosition->setRange(0, 99); m_primaryPosition->setValue(m_link.foreignKeyPrimaryPosition);
    m_role = new QLineEdit(m_link.role);
    layout->addRow(tr("Cardinalité"), m_cardinality);
    layout->addRow(QString(), custom);
    layout->addRow(tr("Place clé étrangère dans clé primaire associée"), m_primaryPosition);
    layout->addRow(tr("Rôle"), m_role);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    auto updateCustom = [this] { const bool enabled = m_cardinality->currentIndex() == 4; m_customMin->setEnabled(enabled); m_customMax->setEnabled(enabled); };
    connect(m_cardinality, &QComboBox::currentIndexChanged, this, updateCustom); updateCustom();
}

Link LinkDialog::link() const
{
    auto result = m_link;
    result.cardinality = static_cast<Cardinality>(m_cardinality->currentIndex());
    result.customMin = m_customMin->text().trimmed();
    result.customMax = m_customMax->text().trimmed();
    result.foreignKeyPrimaryPosition = m_primaryPosition->value();
    result.role = m_role->text().trimmed();
    return result;
}

InheritanceDialog::InheritanceDialog(Inheritance inheritance, QWidget *parent)
    : QDialog(parent), m_inheritance(std::move(inheritance))
{
    setWindowTitle(tr("Héritage")); resize(560, 520);
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout; m_name = new QLineEdit(m_inheritance.name); form->addRow(tr("Nom"), m_name); layout->addLayout(form);
    auto *constraintBox = new QGroupBox(tr("Contrainte sur spécialisation / généralisation"));
    auto *constraintLayout = new QVBoxLayout(constraintBox); m_constraint = new QButtonGroup(this);
    const QStringList labels{tr("Pas de contrainte"), tr("Exclusivité"), tr("Totalité"), tr("Partition")};
    for (int i = 0; i < labels.size(); ++i) { auto *button = new QRadioButton(labels[i]); m_constraint->addButton(button, i); constraintLayout->addWidget(button); }
    if (auto *button = m_constraint->button(static_cast<int>(m_inheritance.constraint))) button->setChecked(true);
    layout->addWidget(constraintBox);
    m_migrate = new QCheckBox(tr("Migration de toutes les rubriques du surtype vers les sous-types"));
    m_migrate->setChecked(m_inheritance.migrateAllFields); layout->addWidget(m_migrate);
    auto *commentBox = new QGroupBox(tr("Commentaire")); auto *commentLayout = new QVBoxLayout(commentBox);
    m_comment = new QTextEdit(m_inheritance.comment); m_comment->setMaximumHeight(80); commentLayout->addWidget(m_comment); layout->addWidget(commentBox);
    auto *sqlBox = new QGroupBox(tr("Script SQL associé à l'héritage")); auto *sqlLayout = new QVBoxLayout(sqlBox);
    m_sqlScript = new QTextEdit(m_inheritance.sqlScript); m_sqlScript->setMaximumHeight(100); sqlLayout->addWidget(m_sqlScript); layout->addWidget(sqlBox);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

Inheritance InheritanceDialog::inheritance() const
{
    auto result = m_inheritance; result.name = m_name->text().trimmed();
    result.constraint = static_cast<InheritanceConstraint>(qMax(0, m_constraint->checkedId()));
    result.migrateAllFields = m_migrate->isChecked(); result.comment = m_comment->toPlainText(); result.sqlScript = m_sqlScript->toPlainText();
    return result;
}

InheritanceLinkDialog::InheritanceLinkDialog(InheritanceLink link, QWidget *parent)
    : QDialog(parent), m_link(std::move(link))
{
    setWindowTitle(tr("Lien héritage")); resize(570, 480);
    auto *layout = new QVBoxLayout(this);
    m_supertype = new QCheckBox(tr("Surtype de l'héritage")); m_supertype->setChecked(m_link.supertype); layout->addWidget(m_supertype);
    auto *form = new QFormLayout;
    m_expression = new QLineEdit(m_link.expression); form->addRow(tr("Expression du critère de spécialisation / généralisation"), m_expression);
    layout->addLayout(form);
    m_display = new QCheckBox(tr("Afficher")); m_display->setChecked(m_link.display); layout->addWidget(m_display);
    m_suffix = new QCheckBox(tr("Suffixer clés étrangères")); m_suffix->setChecked(m_link.suffixForeignKeys); layout->addWidget(m_suffix);
    m_rename = new QLineEdit(m_link.renamedForeignKey); form = new QFormLayout; form->addRow(tr("Renommer clé étrangère"), m_rename);
    m_primaryPosition = new QSpinBox; m_primaryPosition->setRange(0, 99); m_primaryPosition->setValue(m_link.foreignKeyPrimaryPosition);
    form->addRow(tr("Place clé étrangère dans clé primaire associée"), m_primaryPosition); layout->addLayout(form);
    auto *sqlBox = new QGroupBox(tr("Complément SQL pour clés étrangères")); auto *sqlLayout = new QVBoxLayout(sqlBox);
    m_deleteCascade = new QCheckBox(tr("ON DELETE CASCADE")); m_deleteCascade->setChecked(m_link.onDeleteCascade);
    m_updateCascade = new QCheckBox(tr("ON UPDATE CASCADE")); m_updateCascade->setChecked(m_link.onUpdateCascade);
    m_otherSql = new QLineEdit(m_link.otherForeignKeySql); m_otherSql->setPlaceholderText(tr("Autre"));
    sqlLayout->addWidget(m_deleteCascade); sqlLayout->addWidget(m_updateCascade); sqlLayout->addWidget(m_otherSql); layout->addWidget(sqlBox);
    m_smoothBreak = new QCheckBox(tr("Lissage cassure")); m_smoothBreak->setChecked(m_link.smoothBreak); layout->addWidget(m_smoothBreak);
    layout->addStretch(); auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

InheritanceLink InheritanceLinkDialog::link() const
{
    auto result = m_link; result.supertype = m_supertype->isChecked(); result.expression = m_expression->text().trimmed();
    result.display = m_display->isChecked(); result.suffixForeignKeys = m_suffix->isChecked(); result.renamedForeignKey = m_rename->text().trimmed();
    result.foreignKeyPrimaryPosition = m_primaryPosition->value(); result.onDeleteCascade = m_deleteCascade->isChecked();
    result.onUpdateCascade = m_updateCascade->isChecked(); result.otherForeignKeySql = m_otherSql->text().trimmed(); result.smoothBreak = m_smoothBreak->isChecked();
    return result;
}

RuleDialog::RuleDialog(Rule rule, QWidget *parent) : QDialog(parent), m_rule(std::move(rule))
{
    setWindowTitle(tr("Règle")); resize(600, 600);
    auto *layout = new QVBoxLayout(this); auto *form = new QFormLayout;
    m_name = new QLineEdit(m_rule.name); form->addRow(tr("Nom"), m_name); layout->addLayout(form);
    auto *descriptionBox = new QGroupBox(tr("Description")); auto *descriptionLayout = new QVBoxLayout(descriptionBox);
    m_description = new QTextEdit(m_rule.description); descriptionLayout->addWidget(m_description);
    m_showDescription = new QCheckBox(tr("Afficher dans le modèle")); m_showDescription->setChecked(m_rule.showDescription); descriptionLayout->addWidget(m_showDescription);
    m_addDescriptionComment = new QCheckBox(tr("Ajouter en commentaire au début du script SQL ci-dessous"));
    m_addDescriptionComment->setChecked(m_rule.addDescriptionAsSqlComment); descriptionLayout->addWidget(m_addDescriptionComment); layout->addWidget(descriptionBox);
    auto *sqlBox = new QGroupBox(tr("Script SQL associé à la règle")); auto *sqlLayout = new QVBoxLayout(sqlBox);
    m_sqlScript = new QTextEdit(m_rule.sqlScript); sqlLayout->addWidget(m_sqlScript);
    auto *positionLayout = new QHBoxLayout; positionLayout->addWidget(new QLabel(tr("Position :")));
    m_sqlPosition = new QButtonGroup(this); auto *before = new QRadioButton(tr("Avant")); auto *after = new QRadioButton(tr("Après la création des tables"));
    m_sqlPosition->addButton(before, 0); m_sqlPosition->addButton(after, 1); positionLayout->addWidget(before); positionLayout->addWidget(after); positionLayout->addStretch();
    m_sqlPosition->button(static_cast<int>(m_rule.sqlPosition))->setChecked(true); sqlLayout->addLayout(positionLayout);
    m_showSql = new QCheckBox(tr("Afficher dans le modèle")); m_showSql->setChecked(m_rule.showSqlInModel); sqlLayout->addWidget(m_showSql); layout->addWidget(sqlBox);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

Rule RuleDialog::rule() const
{
    auto result = m_rule; result.name = m_name->text().trimmed(); result.description = m_description->toPlainText();
    result.showDescription = m_showDescription->isChecked(); result.addDescriptionAsSqlComment = m_addDescriptionComment->isChecked();
    result.sqlScript = m_sqlScript->toPlainText(); result.sqlPosition = static_cast<SqlRulePosition>(qMax(0, m_sqlPosition->checkedId()));
    result.showSqlInModel = m_showSql->isChecked(); return result;
}
