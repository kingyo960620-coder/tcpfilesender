#include "tcpfilesender.h"

TcpFileSender::TcpFileSender(QWidget *parent)
    : QDialog(parent)
{
    loadSize = 4 *1024;
    totalBytes = 0;
    bytesToWrite = 0;
    bytesWritten = 0;
    clientProgressBar = new QProgressBar;
    clientStatusLabel = new QLabel("客戶端就緒");
    buttonBox = new QDialogButtonBox;
    startButton = new QPushButton("開始");
    quitButton = new QPushButton("退出");
    openButton = new QPushButton("開檔");
    startButton->setEnabled(false);
    buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(openButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::ActionRole);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(clientProgressBar);
    mainLayout->addWidget(clientStatusLabel);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
    setWindowTitle(tr("檔案傳送"));
    connect(openButton, SIGNAL(clicked()), this, SLOT(openfile()));
    connect(startButton, SIGNAL(clicked()), this, SLOT(start()));
    connect(&tcpClient, SIGNAL(connected()), this, SLOT(startTransfer()));
    connect(&tcpClient, SIGNAL(bytesWritten(qint64)), this, SLOT(updateClientProgress(qint64)));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(&tcpClient, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
}

TcpFileSender::~TcpFileSender()
{
}

void TcpFileSender::start(){
    startButton->setEnabled(false);
    bytesWritten = 0;
    clientStatusLabel->setText(tr("連線中..."));
    tcpClient.connectToHost(QHostAddress::LocalHost, 16689);
}
void TcpFileSender::startTransfer(){
    localFile = new QFile(fileName);
    if(!localFile->open(QFile::ReadOnly)){
        QMessageBox::warning(this,tr("應用程式"),
                             tr("無法讀取 %1:\n%2.").arg(fileName).arg(localFile->errorString()));
        return;
    }
    totalBytes = localFile->size();
    QDataStream sendout(&outBlock, QIODevice::WriteOnly);
    sendout.setVersion(QDataStream::Qt_4_6);
    QString currentFile = fileName.right(fileName.size() - fileName.lastIndexOf("/")-1);
    sendout <<qint64(0)<<qint64(0)<<currentFile;
    totalBytes += outBlock.size();

    sendout.device()->seek(0);
    sendout<<totalBytes<<qint64((outBlock.size()-sizeof(qint64)*2));
    bytesToWrite = totalBytes - tcpClient.write(outBlock);
    clientStatusLabel->setText(tr("已連接"));
    qDebug() << currentFile <<totalBytes;
    outBlock.resize(0);
}
void TcpFileSender::openfile(){
    fileName = QFileDialog::getOpenFileName(this);
    if(!fileName.isEmpty()) startButton->setEnabled(true);
}
void TcpFileSender::updateClientProgress(qint64 numBytes){
    bytesWritten+=(int) numBytes;
    if(bytesToWrite > 0){
        outBlock = localFile->read(qMin(bytesToWrite, loadSize));
        bytesToWrite -= (int) tcpClient.write(outBlock);
        outBlock.resize(0);
    }else{
        localFile->close();
    }

    clientProgressBar->setMaximum(totalBytes);
    clientProgressBar->setValue(bytesWritten);
    clientStatusLabel->setText(tr("已傳送 %1 Bytes").arg(bytesWritten));

    if(bytesWritten == totalBytes){
        clientStatusLabel->setText(tr("傳送完成"));
        localFile->deleteLater();   // 順便解決 new QFile 沒釋放的問題
        tcpClient.close();
        startButton->setEnabled(true);   // 讓使用者可以再傳一次
    }
}
void TcpFileSender::displayError(QAbstractSocket::SocketError){
    QMessageBox::information(this, tr("網路錯誤"),
                             tr("產生如下錯誤: %1").arg(tcpClient.errorString()));
    tcpClient.close();
    clientProgressBar->reset();
    clientStatusLabel->setText(tr("客戶端就緒"));
    startButton->setEnabled(true);
}