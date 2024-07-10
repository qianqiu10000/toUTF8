#include<QFile>
#include<QDir>
#include<QTextCodec>

#include<QChar>
bool translateFile(const QString &file)
{
    //打开源文件
    QFile origin(file);
    if(!origin.open(QFile::ReadOnly))
    {
        return false;
    }

    //在源文件路径下创建一个UTF8文件夹，
    QFileInfo info(origin);
    QDir dir = info.dir();
    if(!dir.exists("UTF8")) {
        dir.mkdir("UTF8");
    }

    //目标文件放在上面的UTF8文件夹里，名字与源文件相同
    info.fileName();
    QString to = dir.path() + "/UTF8/" + info.fileName();
    QFile target(to);
    if(!target.open(QFile::WriteOnly))
    {
        origin.close();
        return false;
    }

    //把ACSII字符排除, 值[0,127]各种编码通用，难以判断
    qint64 num = 0;
    char byte = 0;
    while ((!origin.atEnd()) && (byte <= 127) && byte >= 0) {
        byte = origin.read(1).at(0);
        num++;
    }

    //是否ANSI编码标记，默认不是
    bool ansi = false;

    //如果文件结尾了，说明前面全部是ACSII字符,不用转码，直接复制
    if(!origin.atEnd())
    {
        //根据前3个字节判断编码是不是ANSI
        origin.seek(num - 1);
        QByteArray header = origin.read(3);
        QTextCodec *codex = QTextCodec::codecForName("UTF-8");
        QTextCodec::ConverterState state;
        codex->toUnicode(header.data(), header.size(), &state);

        //UTF8汉字三字节编码，如果无效字符数大于0，说明不是UTF8
        ansi = state.invalidChars;
    }

    //文件指针回到开始
    origin.reset();

    //如果是ansi编码，进行转换
    //否则直接复制
    if(ansi)
    {
        while(!origin.atEnd())
        {
            QString line = QString::fromLocal8Bit(origin.readLine());
            target.write(line.toUtf8());
        }
    }
    else
    {
        while(!origin.atEnd()) {
            target.write(origin.readLine());
        }
    }
    target.close();
    origin.close();

    return true;
}

int  main()
{

    QDir dir;
    QFileInfoList infos = dir.entryInfoList(QDir::Files);

    QStringList files;
    for(int i = 0; i < infos.count(); i++)
    {
        QFileInfo info = infos.at(i);
        QString suffix = info.suffix();
        if(suffix == "h" || suffix == "cpp" || suffix == "c") {
            files.append(info.absoluteFilePath());
        }
    }

    for(int i = 0; i < files.count(); i++)
    {
        translateFile(files.at(i));
    }

    return 0;
}
