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
    // Name of Quiz to be shown. Example: Quiz 1 or Card 1.2.1
    QString name() const { return _name; }
    // Status of quiz attempt. 0 = Not started, 1 = In Progress, 2 = Finished
    int status() const { return _status; }
    // How many were correct
    int correct() const { return _correct; }
    // How far through Quiz the student is
    int position() const { return _position; }
    // Total questions for this quiz
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
    Q_INVOKABLE QString getName(int index) {
        if (index < 0 || index >= _quizs.count())
            return "";
        const QuizInfo &quiz = _quizs[index];
        return quiz.name();
    }
    Q_INVOKABLE int getStatus(int index) {
        if (index < 0 || index >= _quizs.count())
            return 2;
        const QuizInfo &quiz = _quizs[index];
        return quiz.status();
    }
    Q_INVOKABLE int getPosition(int index) {
        if (index < 0 || index >= _quizs.count())
            return -1;
        const QuizInfo &quiz = _quizs[index];
        return quiz.position();
    }
    Q_INVOKABLE int getTotal(int index) {
        if (index < 0 || index >= _quizs.count())
            return -1;
        const QuizInfo &quiz = _quizs[index];
        return quiz.total();
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
