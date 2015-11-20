#ifndef ACCOUNTINFO_H
#define ACCOUNTINFO_H

#include <QObject>
#include <QAbstractListModel>
#include <QDebug>

class QuizInfo
{
public:
    QuizInfo(const QString &name, const int &status, const int &correct, const int &position, const int &total)
        : _name(name), _status(status), _correct(correct), _position(position), _total(total)
    { }
    QString name() const { return _name; }
    int status() const { return _status; }
    int correct() const { return _correct; }
    int position() const { return _position; }
    int total() const { return _total; }
private:
    QString _name;
    int _status;
    int _correct;
    int _position;
    int _total;
};

class QuizModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum QuizRoles {
        NameRole = Qt::UserRole + 1,
        StatusRole,
        CorrectRole,
        PositionRole,
        TotalRole
    };

    QuizModel(QObject *parent = 0)
    : QAbstractListModel(parent)
    { }

    void addQuiz(const QuizInfo &quiz)
    {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        _quizs << quiz;
        endInsertRows();
    }
    int rowCount(const QModelIndex & parent = QModelIndex()) const {
        Q_UNUSED(parent);
        return _quizs.count();
    }
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const {
        if (index.row() < 0 || index.row() >= _quizs.count())
            return QVariant();

        const QuizInfo &quiz = _quizs[index.row()];
        if (role == NameRole)
            return quiz.name();
        else if (role == StatusRole)
            return quiz.status();
        else if (role == CorrectRole)
            return quiz.correct();
        else if (role == PositionRole)
            return quiz.position();
        else if (role == TotalRole)
            return quiz.total();
        return QVariant();
    }
protected:
    QHash<int, QByteArray> roleNames() const {
        QHash<int, QByteArray> roles;
        roles[NameRole] = "name";
        roles[StatusRole] = "status";
        roles[CorrectRole] = "correct";
        roles[PositionRole] = "position";
        roles[TotalRole] = "total";
        return roles;
    }
private:
    QList<QuizInfo> _quizs;
};


class AccountInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString user READ user WRITE setUser NOTIFY userChanged)

public:
    explicit AccountInfo(QObject *parent = 0);
    QString user() const { return _user; }
    void setUser(QString &user) { _user = user; userChanged(user); }

signals:
    void userChanged(const QString &newUser);

public slots:

private:
    QString _user;
};

#endif // ACCOUNTINFO_H
