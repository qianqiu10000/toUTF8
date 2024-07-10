#include<QFile>
#include<QDir>
#include<QTextCodec>
#include<QTextStream>


enum Type {
    ANSI,
    UTF8,
    UTF8BOM,
    UTF16LE,
    UTF16BE
};

bool translateFile(const QString &file)
{
    //打开源文件
    QFile origin(file);
    if(!origin.open(QFile::ReadOnly))
    {
        return false;
    }

    //如果源文件是空的
    if(origin.atEnd()) {
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

    //判断源文件类型，默认UTF8
    Type type = UTF8;

    //前三个字节判断一下
    QByteArray utf = origin.read(3);
    quint8 byte0 = utf.at(0);
    quint8 byte1 = utf.at(1);
    quint8 byte2 = utf.at(2);

    origin.reset();
    //如果是UTF16LE编码
    if(byte0 == 0xFF && byte1 == 0xFE) {
        type = UTF16LE;
    }
    else if(byte0 == 0xFE && byte1 == 0xFF) {
        type = UTF16BE;
    }
    else if(byte0 == 0xEF && byte1 == 0xBB && byte2 == 0xBF) {
        type = UTF8BOM;
    }
    else {                                              //如果没有特征字节，需要试错
        //把ACSII字符跳过, 值[0,127]各种编码通用，难以判断
        qint64 num = 0;
        char byte = 0;
        while ((!origin.atEnd()) && (byte <= 127) && byte >= 0) {
            byte = origin.read(1).at(0);
            num++;
        }

        //如果文件不在结尾，说明含有非ACSII码字符，一般是中文
        if(!origin.atEnd())
        {
            QTextCodec *codec = QTextCodec::codecForName("UTF-8");
            //根据前3个字节判断
            origin.seek(num - 1);
            QByteArray header = origin.read(3);
            QTextCodec::ConverterState state;
            codec->toUnicode(header.data(), header.size(), &state);

            //UTF8汉字三字节编码，如果无效字符数大于0，说明不是UTF8
            if(state.invalidChars) {
                type = ANSI;
            }

        }
    }

    origin.reset();
    //编码转换，一行一行进行
    switch (type) {
        case UTF16LE:
            {
                QTextStream read(&origin);
                read.setCodec(QTextCodec::codecForName("UTF-16LE"));
                QTextStream write(&target);
                write.setCodec(QTextCodec::codecForName("UTF-8"));
                write << read.readAll();
            }
            break;
        case UTF16BE:
            {
                QTextStream read(&origin);
                read.setCodec(QTextCodec::codecForName("UTF-16BE"));
                QTextStream write(&target);
                write.setCodec(QTextCodec::codecForName("UTF-8"));
                write << read.readAll();
            }
            break;
        case ANSI:
            {
                QTextStream read(&origin);
                read.setCodec(QTextCodec::codecForLocale());
                QTextStream write(&target);
                write.setCodec(QTextCodec::codecForName("UTF-8"));
                write << read.readAll();
            }
            break;
        case UTF8BOM:
            {
                QTextStream read(&origin);
                read.setCodec(QTextCodec::codecForName("UTF-8"));
                read.setGenerateByteOrderMark(true);
                QTextStream write(&target);
                write.setCodec(QTextCodec::codecForName("UTF-8"));
                write << read.readAll();
            }
            break;
        default:
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
