#include <QApplication>
#include <QQmlApplicationEngine>
#include <QNetworkProxyFactory>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickWindow>
#include <QMessageBox>
#include "client.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s\n", localMsg.constData());
        break;
    case QtInfoMsg:
        fprintf(stderr, "Info: %s\n", localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", localMsg.constData());
        abort();
    }
    fflush(stderr);
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(myMessageOutput);
    QApplication::setAttribute(Qt::AA_X11InitThreads, true);
    QApplication app(argc, argv);
    app.setOrganizationName("PRSHS");
    app.setOrganizationDomain("prshs.edu.au");
    app.setApplicationName("Quiz");
    app.setApplicationVersion("0.0.2");

    // Use system proxy except where not possible
    QNetworkProxyFactory::setUseSystemConfiguration(false);

    Client client;

    QQmlApplicationEngine engine;
    QQmlContext *context = engine.rootContext();
    context->setContextProperty("client", &client);
    context->setContextProperty("info", (&client)->accountInfo);

    QScopedPointer<QQmlComponent> comp(new QQmlComponent(&engine));
    comp->loadUrl(QUrl(QStringLiteral("qrc:/main.qml")));
    if (comp->status() == QQmlComponent::Error) {
        QMessageBox::information(nullptr, "Error", qPrintable(comp->errorString()), QMessageBox::Ok);
        return 0;
    }
    QQuickWindow *window = qobject_cast<QQuickWindow *>(comp->create());
    window->setMinimumHeight(480);
    window->setMinimumWidth(640);
    //window->showMaximized();
    window->show();

    int ret = app.exec();

    delete window;
    return ret;
}

