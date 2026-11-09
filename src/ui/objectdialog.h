#pragma once

#include "core/model.h"

#include <QDialog>

class QLineEdit;
class QTableWidget;
class QButtonGroup;
class QSpinBox;
class QTextEdit;

class FieldDialog : public QDialog {
    Q_OBJECT
public:
    explicit FieldDialog(looping::Field field, QWidget *parent = nullptr);
    looping::Field field() const;

private:
    void updateAvailability();
    looping::Field m_field;
    QLineEdit *m_name;
    QLineEdit *m_logicalName;
    QButtonGroup *m_encoding;
    QButtonGroup *m_textShape;
    QButtonGroup *m_directType;
    QSpinBox *m_length;
    QSpinBox *m_precision;
    QSpinBox *m_scale;
    QLineEdit *m_collation;
    QLineEdit *m_freeType;
    QLineEdit *m_complement;
    class QCheckBox *m_primaryKey;
    class QCheckBox *m_notNull;
    class QCheckBox *m_unique;
    QTextEdit *m_comment;
};

class ObjectDialog : public QDialog {
    Q_OBJECT
public:
    explicit ObjectDialog(const QString &title, QWidget *parent = nullptr);
    void setObject(const QString &name, const QString &logicalName, const QVector<looping::Field> &fields);
    QString objectName() const;
    QString logicalName() const;
    QVector<looping::Field> fields() const;

private slots:
    void addField();
    void modifyField();
    void removeField();

private:
    void appendFieldRow(const looping::Field &field);
    looping::Field fieldAt(int row) const;
    void setFieldAt(int row, const looping::Field &field);
    QLineEdit *m_name;
    QLineEdit *m_logicalName;
    QTableWidget *m_fields;
};

class LinkDialog : public QDialog {
    Q_OBJECT
public:
    explicit LinkDialog(looping::Link link, QWidget *parent = nullptr);
    looping::Link link() const;

private:
    looping::Link m_link;
    class QComboBox *m_cardinality;
    QLineEdit *m_role;
    QLineEdit *m_customMin;
    QLineEdit *m_customMax;
    class QSpinBox *m_primaryPosition;
};

class InheritanceDialog : public QDialog {
    Q_OBJECT
public:
    explicit InheritanceDialog(looping::Inheritance inheritance, QWidget *parent = nullptr);
    looping::Inheritance inheritance() const;
private:
    looping::Inheritance m_inheritance;
    QLineEdit *m_name;
    QButtonGroup *m_constraint;
    class QCheckBox *m_migrate;
    QTextEdit *m_comment;
    QTextEdit *m_sqlScript;
};

class InheritanceLinkDialog : public QDialog {
    Q_OBJECT
public:
    explicit InheritanceLinkDialog(looping::InheritanceLink link, QWidget *parent = nullptr);
    looping::InheritanceLink link() const;
private:
    looping::InheritanceLink m_link;
    class QCheckBox *m_supertype;
    QLineEdit *m_expression;
    class QCheckBox *m_display;
    class QCheckBox *m_suffix;
    QLineEdit *m_rename;
    QSpinBox *m_primaryPosition;
    class QCheckBox *m_deleteCascade;
    class QCheckBox *m_updateCascade;
    QLineEdit *m_otherSql;
    class QCheckBox *m_smoothBreak;
};

class RuleDialog : public QDialog {
    Q_OBJECT
public:
    explicit RuleDialog(looping::Rule rule, QWidget *parent = nullptr);
    looping::Rule rule() const;
private:
    looping::Rule m_rule;
    QLineEdit *m_name;
    QTextEdit *m_description;
    class QCheckBox *m_showDescription;
    class QCheckBox *m_addDescriptionComment;
    QTextEdit *m_sqlScript;
    QButtonGroup *m_sqlPosition;
    class QCheckBox *m_showSql;
};
