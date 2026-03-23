#include "widget.h"
#include "ui_widget.h"
struct Read_from_Plc			//定义结构体
{
    short intVal[7];			//整型
    bool  boolVal[9];			//布尔型
}rfp;

struct Writie_to_Plc			//定义结构体
{
    short intVal[4];			//整型
    bool  boolVal[9];			//布尔型
}wtp;
Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    loadParams_Axi_1();



    setWindowTitle("气溶胶设备上位机软件");  // 设置窗口标题
    modbusClient = new QModbusTcpClient(this);
    if (nullptr == modbusClient )
    {
        qDebug() << "无法创建 Modbus 客户端";
    }
    else
    {
        connect(modbusClient, &QModbusClient::stateChanged,this, &Widget::onModbusStateChanged);
    }
}

Widget::~Widget()
{
    if (modbusClient)
    {
        modbusClient->disconnectDevice();
    }
    delete modbusClient;
    delete ui;
}

void Widget::onModbusStateChanged(int state)
{
    //判断Modbus设备连接是否处于连接状态
    if (state == QModbusDevice::UnconnectedState)
    {
        qDebug() << "TCP Client连接到Server 未连接";
        ui->lineEdit_plc_connect_status->setText("未连接");
        ui->btn_connect_plc->setText(tr("未连接"));

    }
    else if (state == QModbusDevice::ConnectingState)
    {
        qDebug() << "TCP Client正在连接Server";
    }
    else if (state == QModbusDevice::ConnectedState)
    {
        qDebug() << "TCP Client已经连接到Server";
        ui->lineEdit_plc_connect_status->setText("已连接");
        ui->btn_connect_plc->setText(tr("断开连接"));

    }
    else if (state == QModbusDevice::ClosingState)
    {
        qDebug() << "设备已经被关闭";
    }
}

void Widget::on_btn_connect_plc_clicked()
{


    const QString qv= (ui->lineEdit_plc_addr->text()) ;
    qDebug() <<ui->lineEdit_plc_addr->text();
    // 连接到Modbus服务器
    if (!modbusClient)
    {
        return;
    }
    if (modbusClient->state() != QModbusDevice::ConnectedState)
    {
        modbusClient->setConnectionParameter(QModbusTcpClient::NetworkAddressParameter,ui->lineEdit_plc_addr->text()); // 服务器IP
        modbusClient->setConnectionParameter(QModbusTcpClient::NetworkPortParameter, 502); // 默认Modbus TCP端口

        //设置超时时间
        modbusClient->setTimeout(1000); //1秒
        //设置失败重试次数
        modbusClient->setNumberOfRetries(3);

        //连接到服务端
        bool ok = modbusClient->connectDevice();
        if (ok)
        {
            ui->lineEdit_plc_connect_status->setText("连接中");
            _pTimerUpdate = new QTimer();
            connect(_pTimerUpdate, SIGNAL(timeout()), this, SLOT(readAxisState_1to5()));
            _pTimerUpdate->start(2000);

        }
    }
    else
    {
        //断开连接
        modbusClient->disconnectDevice();
        ui->btn_connect_plc->setText(tr("连接"));
    }

}



void Widget::onReadFinished3()
{
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)

    {
        qDebug() << "REply erro";
        return;
    }

    // 处理读取的Modbus数据
    if (reply->error() == QModbusDevice::NoError)
    {
        const QModbusDataUnit unit = reply->result();

        // 气缸
        qDebug()<<"-----------"<<unit.value(0);
        int swflag;
        swflag=unit.value(0);

        if(unit.value(0)==0)
        {
          ui->lineEdit_Mode->setText("关机");
        }
        else if(unit.value(0)==1)
        {
            ui->lineEdit_Mode->setText("初始化");
        }
        else if(unit.value(0)==2)
        {
            ui->lineEdit_Mode->setText("手动");
        }
        else if(unit.value(0)==3)
        {
            ui->lineEdit_Mode->setText("自动");
        }
        else if(unit.value(0)==4)
        {
            ui->lineEdit_Mode->setText("自动");
        }
        else if(unit.value(0)==5)
        {
            ui->lineEdit_Mode->setText("故障");
        }
        else
            ui->lineEdit_Mode->setText("关机");

    }




    else
    {
        qDebug() << "Modbus read error:" << reply->errorString();
    }

    reply->deleteLater();
}


void Widget::onWriteFinished()
{
    QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) return;

    // 处理写入结果
    if (reply->error() == QModbusDevice::NoError) {
        qDebug() << "Write operation succeeded.";
    } else {
        qDebug() << "Modbus write error3:" << reply->errorString();
    }

    reply->deleteLater();
}



void Widget::readAxisState_1to5()
{
    if (m_readingAxis_1to5) return;
    m_readingAxis_1to5 = true;

    const int serverId  = 1;
    const int startAddr = 400;
    const int count     = 100;

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddr, count);
    QModbusReply *reply = modbusClient->sendReadRequest(readUnit, serverId);

    if (!reply) {
        qDebug() << "sendReadRequest failed";
        m_readingAxis_1to5 = false;
        return;
    }

    auto setGreen = [](QLineEdit* e, bool on){
        e->setStyleSheet(on ? "QLineEdit{ background:#00ff00; color:#000; }" : "");
    };

    auto handle = [this, reply, setGreen]() {
        m_readingAxis_1to5 = false;

        if (reply->error() != QModbusDevice::NoError) {
            qDebug() << "Modbus read error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        const QModbusDataUnit unit = reply->result();

        //==================== 轴1：unit.value(0..19) ====================
        ui->lineEdit_AxiErri_Axi_1->setText(QString::number(unit.value(0)));
        ui->lineEdit_SeroNo_Axi_1->setText(QString::number(unit.value(1)));
        setGreen(ui->lineEdit_Enable_Axi_1,            unit.value(2)  == 1);
        setGreen(ui->lineEdit_Estop_Axi_1,             unit.value(3)  == 1);
        setGreen(ui->lineEdit_Moving_Axi_1,            unit.value(4)  == 1);
        setGreen(ui->lineEdit_Err_Axi_1,               unit.value(5)  == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_1,  unit.value(6)  == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_1,  unit.value(7)  == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_1,  unit.value(8)  == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_1,  unit.value(9)  == 1);
        setGreen(ui->lineEdit_Ready_Axi_1,             unit.value(10) == 1);
        setGreen(ui->lineEdit_Homed_Axi_1,             unit.value(11) == 1);
        ui->lineEdit_CurPos_Axi_1->setText(QString::number(unit.value(12) / 10.0));
        ui->lineEdit_SetPos_Axi_1->setText(QString::number(unit.value(13) / 10.0));
        ui->lineEdit_CurVel_Axi_1->setText(QString::number(unit.value(14) / 10.0));
        ui->lineEdit_JogMode_Axi_1->setText(unit.value(15) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_1->setText(unit.value(16) == 0 ? "软限位关" : "软限位开");

        //==================== 轴2：unit.value(20..39) ====================
        ui->lineEdit_AxiErri_Axi_2->setText(QString::number(unit.value(20)));
        ui->lineEdit_SeroNo_Axi_2->setText(QString::number(unit.value(21)));
        setGreen(ui->lineEdit_Enable_Axi_2,            unit.value(22) == 1);
        setGreen(ui->lineEdit_Estop_Axi_2,             unit.value(23) == 1);
        setGreen(ui->lineEdit_Moving_Axi_2,            unit.value(24) == 1);
        setGreen(ui->lineEdit_Err_Axi_2,               unit.value(25) == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_2,  unit.value(26) == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_2,  unit.value(27) == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_2,  unit.value(28) == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_2,  unit.value(29) == 1);
        setGreen(ui->lineEdit_Ready_Axi_2,             unit.value(30) == 1);
        setGreen(ui->lineEdit_Homed_Axi_2,             unit.value(31) == 1);
        ui->lineEdit_CurPos_Axi_2->setText(QString::number(unit.value(32) / 10.0));
        ui->lineEdit_SetPos_Axi_2->setText(QString::number(unit.value(33) / 10.0));
        ui->lineEdit_CurVel_Axi_2->setText(QString::number(unit.value(34) / 10.0));
        ui->lineEdit_JogMode_Axi_2->setText(unit.value(35) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_2->setText(unit.value(36) == 0 ? "软限位关" : "软限位开");

        //==================== 轴3：unit.value(40..59) ====================
        ui->lineEdit_AxiErri_Axi_3->setText(QString::number(unit.value(40)));
        ui->lineEdit_SeroNo_Axi_3->setText(QString::number(unit.value(41)));
        setGreen(ui->lineEdit_Enable_Axi_3,            unit.value(42) == 1);
        setGreen(ui->lineEdit_Estop_Axi_3,             unit.value(43) == 1);
        setGreen(ui->lineEdit_Moving_Axi_3,            unit.value(44) == 1);
        setGreen(ui->lineEdit_Err_Axi_3,               unit.value(45) == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_3,  unit.value(46) == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_3,  unit.value(47) == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_3,  unit.value(48) == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_3,  unit.value(49) == 1);
        setGreen(ui->lineEdit_Ready_Axi_3,             unit.value(50) == 1);
        setGreen(ui->lineEdit_Homed_Axi_3,             unit.value(51) == 1);
        ui->lineEdit_CurPos_Axi_3->setText(QString::number(unit.value(52) / 10.0));

        ui->lineEdit_SetPos_Axi_3->setText(QString::number(unit.value(53) / 10.0));
        ui->lineEdit_CurVel_Axi_3->setText(QString::number(unit.value(54) / 10.0));
        ui->lineEdit_JogMode_Axi_3->setText(unit.value(55) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_3->setText(unit.value(56) == 0 ? "软限位关" : "软限位开");

        //==================== 轴4：unit.value(60..79) ====================
        ui->lineEdit_AxiErri_Axi_4->setText(QString::number(unit.value(60)));
        ui->lineEdit_SeroNo_Axi_4->setText(QString::number(unit.value(61)));
        setGreen(ui->lineEdit_Enable_Axi_4,            unit.value(62) == 1);
        setGreen(ui->lineEdit_Estop_Axi_4,             unit.value(63) == 1);
        setGreen(ui->lineEdit_Moving_Axi_4,            unit.value(64) == 1);
        setGreen(ui->lineEdit_Err_Axi_4,               unit.value(65) == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_4,  unit.value(66) == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_4,  unit.value(67) == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_4,  unit.value(68) == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_4,  unit.value(69) == 1);
        setGreen(ui->lineEdit_Ready_Axi_4,             unit.value(70) == 1);
        setGreen(ui->lineEdit_Homed_Axi_4,             unit.value(71) == 1);
        ui->lineEdit_CurPos_Axi_4->setText(QString::number(unit.value(72) / 10.0));
        ui->lineEdit_SetPos_Axi_4->setText(QString::number(unit.value(73) / 10.0));
        ui->lineEdit_CurVel_Axi_4->setText(QString::number(unit.value(74) / 10.0));
        ui->lineEdit_JogMode_Axi_4->setText(unit.value(75) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_4->setText(unit.value(76) == 0 ? "软限位关" : "软限位开");

        //==================== 轴5：unit.value(80..99) ====================
        ui->lineEdit_AxiErri_Axi_5->setText(QString::number(unit.value(80)));
        ui->lineEdit_SeroNo_Axi_5->setText(QString::number(unit.value(81)));
        setGreen(ui->lineEdit_Enable_Axi_5,            unit.value(82) == 1);
        setGreen(ui->lineEdit_Estop_Axi_5,             unit.value(83) == 1);
        setGreen(ui->lineEdit_Moving_Axi_5,            unit.value(84) == 1);
        setGreen(ui->lineEdit_Err_Axi_5,               unit.value(85) == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_5,  unit.value(86) == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_5,  unit.value(87) == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_5,  unit.value(88) == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_5,  unit.value(89) == 1);
        setGreen(ui->lineEdit_Ready_Axi_5,             unit.value(90) == 1);
        setGreen(ui->lineEdit_Homed_Axi_5,             unit.value(91) == 1);
        ui->lineEdit_CurPos_Axi_5->setText(QString::number(unit.value(92) / 10.0));
        ui->lineEdit_SetPos_Axi_5->setText(QString::number(unit.value(93) / 10.0));
        ui->lineEdit_CurVel_Axi_5->setText(QString::number(unit.value(94) / 10.0));
        ui->lineEdit_JogMode_Axi_5->setText(unit.value(95) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_5->setText(unit.value(96) == 0 ? "软限位关" : "软限位开");

        ui->lineEdit_Get_Box_Sum->setText(QString::number(unit.value(97)));
        ui->lineEdit_Get_Cap_Sum->setText(QString::number(unit.value(98)));
        ui->lineEdit_Store_Sum->setText(QString::number(unit.value(99)));

        reply->deleteLater();

        this->readAxisState_6to10();
    };

    connect(reply, &QModbusReply::finished, this, handle);
    if (reply->isFinished()) handle();
}


void Widget::readAxisState_6to10()
{
    if (m_readingAxis_6to10) return;
    m_readingAxis_6to10 = true;

    const int serverId  = 1;
    const int startAddr = 500;
    const int count     = 100;

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddr, count);
    QModbusReply *reply = modbusClient->sendReadRequest(readUnit, serverId);

    if (!reply) {
        qDebug() << "sendReadRequest failed";
        m_readingAxis_6to10 = false;
        return;
    }

    auto setGreen = [](QLineEdit* e, bool on){
        e->setStyleSheet(on ? "QLineEdit{ background:#00ff00; color:#000; }" : "");
    };

    auto handle = [this, reply, setGreen]() {
        m_readingAxis_6to10 = false;

        if (reply->error() != QModbusDevice::NoError) {
            qDebug() << "Modbus read error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        const QModbusDataUnit unit = reply->result();

        //==================== 轴6：unit.value(0..19) ====================
        ui->lineEdit_AxiErri_Axi_6->setText(QString::number(unit.value(0)));
        ui->lineEdit_SeroNo_Axi_6->setText(QString::number(unit.value(1)));
        setGreen(ui->lineEdit_Enable_Axi_6,            unit.value(2)  == 1);
        setGreen(ui->lineEdit_Estop_Axi_6,             unit.value(3)  == 1);
        setGreen(ui->lineEdit_Moving_Axi_6,            unit.value(4)  == 1);
        setGreen(ui->lineEdit_Err_Axi_6,               unit.value(5)  == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_6,  unit.value(6)  == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_6,  unit.value(7)  == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_6,  unit.value(8)  == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_6,  unit.value(9)  == 1);
        setGreen(ui->lineEdit_Ready_Axi_6,             unit.value(10) == 1);
        setGreen(ui->lineEdit_Homed_Axi_6,             unit.value(11) == 1);
        ui->lineEdit_CurPos_Axi_6->setText(QString::number(unit.value(12) / 10.0));
        ui->lineEdit_SetPos_Axi_6->setText(QString::number(unit.value(13) / 10.0));
        ui->lineEdit_CurVel_Axi_6->setText(QString::number(unit.value(14) / 10.0));
        ui->lineEdit_JogMode_Axi_6->setText(unit.value(15) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_6->setText(unit.value(16) == 0 ? "软限位关" : "软限位开");

        //==================== 轴7：unit.value(20..39) ====================
        ui->lineEdit_AxiErri_Axi_7->setText(QString::number(unit.value(20)));
        ui->lineEdit_SeroNo_Axi_7->setText(QString::number(unit.value(21)));
        setGreen(ui->lineEdit_Enable_Axi_7,            unit.value(22) == 1);
        setGreen(ui->lineEdit_Estop_Axi_7,             unit.value(23) == 1);
        setGreen(ui->lineEdit_Moving_Axi_7,            unit.value(24) == 1);
        setGreen(ui->lineEdit_Err_Axi_7,               unit.value(25) == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_7,  unit.value(26) == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_7,  unit.value(27) == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_7,  unit.value(28) == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_7,  unit.value(29) == 1);
        setGreen(ui->lineEdit_Ready_Axi_7,             unit.value(30) == 1);
        setGreen(ui->lineEdit_Homed_Axi_7,             unit.value(31) == 1);
        ui->lineEdit_CurPos_Axi_7->setText(QString::number(unit.value(32) / 10.0));
        ui->lineEdit_SetPos_Axi_7->setText(QString::number(unit.value(33) / 10.0));
        ui->lineEdit_CurVel_Axi_7->setText(QString::number(unit.value(34) / 10.0));
        ui->lineEdit_JogMode_Axi_7->setText(unit.value(35) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_7->setText(unit.value(36) == 0 ? "软限位关" : "软限位开");

        //==================== 轴8：unit.value(40..59) ====================
        ui->lineEdit_AxiErri_Axi_8->setText(QString::number(unit.value(40)));
        ui->lineEdit_SeroNo_Axi_8->setText(QString::number(unit.value(41)));
        setGreen(ui->lineEdit_Enable_Axi_8,            unit.value(42) == 1);
        setGreen(ui->lineEdit_Estop_Axi_8,             unit.value(43) == 1);
        setGreen(ui->lineEdit_Moving_Axi_8,            unit.value(44) == 1);
        setGreen(ui->lineEdit_Err_Axi_8,               unit.value(45) == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_8,  unit.value(46) == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_8,  unit.value(47) == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_8,  unit.value(48) == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_8,  unit.value(49) == 1);
        setGreen(ui->lineEdit_Ready_Axi_8,             unit.value(50) == 1);
        setGreen(ui->lineEdit_Homed_Axi_8,             unit.value(51) == 1);
        ui->lineEdit_CurPos_Axi_8->setText(QString::number(unit.value(52) / 10.0));
        ui->lineEdit_SetPos_Axi_8->setText(QString::number(unit.value(53) / 10.0));
        ui->lineEdit_CurVel_Axi_8->setText(QString::number(unit.value(54) / 10.0));
        ui->lineEdit_JogMode_Axi_8->setText(unit.value(55) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_8->setText(unit.value(56) == 0 ? "软限位关" : "软限位开");

        //==================== 轴9：unit.value(60..79) ====================
        ui->lineEdit_AxiErri_Axi_9->setText(QString::number(unit.value(60)));
        ui->lineEdit_SeroNo_Axi_9->setText(QString::number(unit.value(61)));
        setGreen(ui->lineEdit_Enable_Axi_9,            unit.value(62) == 1);
        setGreen(ui->lineEdit_Estop_Axi_9,             unit.value(63) == 1);
        setGreen(ui->lineEdit_Moving_Axi_9,            unit.value(64) == 1);
        setGreen(ui->lineEdit_Err_Axi_9,               unit.value(65) == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_9,  unit.value(66) == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_9,  unit.value(67) == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_9,  unit.value(68) == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_9,  unit.value(69) == 1);
        setGreen(ui->lineEdit_Ready_Axi_9,             unit.value(70) == 1);
        setGreen(ui->lineEdit_Homed_Axi_9,             unit.value(71) == 1);
        ui->lineEdit_CurPos_Axi_9->setText(QString::number(unit.value(72) / 10.0));
        ui->lineEdit_SetPos_Axi_9->setText(QString::number(unit.value(73) / 10.0));
        ui->lineEdit_CurVel_Axi_9->setText(QString::number(unit.value(74) / 10.0));
        ui->lineEdit_JogMode_Axi_9->setText(unit.value(75) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_9->setText(unit.value(76) == 0 ? "软限位关" : "软限位开");

        //==================== 轴10：unit.value(80..99) ====================
        ui->lineEdit_AxiErri_Axi_10->setText(QString::number(unit.value(80)));
        ui->lineEdit_SeroNo_Axi_10->setText(QString::number(unit.value(81)));
        setGreen(ui->lineEdit_Enable_Axi_10,            unit.value(82) == 1);
        setGreen(ui->lineEdit_Estop_Axi_10,             unit.value(83) == 1);
        setGreen(ui->lineEdit_Moving_Axi_10,            unit.value(84) == 1);
        setGreen(ui->lineEdit_Err_Axi_10,               unit.value(85) == 1);
        setGreen(ui->lineEdit_Hard_AxiPosLimit_Axi_10,  unit.value(86) == 1);
        setGreen(ui->lineEdit_Hard_AxiNegLimit_Axi_10,  unit.value(87) == 1);
        setGreen(ui->lineEdit_Soft_AxiPosLimit_Axi_10,  unit.value(88) == 1);
        setGreen(ui->lineEdit_Soft_AxiNegLimit_Axi_10,  unit.value(89) == 1);
        setGreen(ui->lineEdit_Ready_Axi_10,             unit.value(90) == 1);
        setGreen(ui->lineEdit_Homed_Axi_10,             unit.value(91) == 1);
        ui->lineEdit_CurPos_Axi_10->setText(QString::number(unit.value(92) / 10.0));
        ui->lineEdit_SetPos_Axi_10->setText(QString::number(unit.value(93) / 10.0));
        ui->lineEdit_CurVel_Axi_10->setText(QString::number(unit.value(94) / 10.0));
        ui->lineEdit_JogMode_Axi_10->setText(unit.value(95) == 0 ? "连续点动" : "寸动点动");
        ui->lineEdit_SoftLimitSet__Axi_10->setText(unit.value(96) == 0 ? "软限位关" : "软限位开");
        ui->lineEdit_iRollupSenser->setText(QString::number(unit.value(98)));
        ui->lineEdit_iUnwindingSenser->setText(QString::number(unit.value(97)));
        reply->deleteLater();
        this->readPlcState();
    };

    connect(reply, &QModbusReply::finished, this, handle);
    if (reply->isFinished()) handle();
}



void Widget::readPlcState()
{
    if (m_readingPlc) return;
    m_readingPlc = true;

    const int serverId  = 1;
    const int startAddr = 320;
    const int count     = 50;

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddr, count);
    QModbusReply *reply = modbusClient->sendReadRequest(readUnit, serverId);

    if (!reply) {
        qDebug() << "sendReadRequest failed";
        m_readingPlc = false;
        return;
    }

    auto setGreen = [](QLineEdit* e, bool on){
        e->setStyleSheet(on ? "QLineEdit{ background:#00ff00; color:#000; }" : "");
    };

    auto handle = [this, reply, setGreen]() {
        m_readingPlc = false;

        if (reply->error() != QModbusDevice::NoError) {
            qDebug() << "Modbus read error:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        const QModbusDataUnit unit = reply->result();

        //==================== 轴1：unit.value(0..19) ====================
        if (unit.value(0)==1) ui->lineEdit_CylinderCmd_1->setText("伸出");
        if (unit.value(0)==2) ui->lineEdit_CylinderCmd_1->setText("缩回");
        if (unit.value(1)==1) ui->lineEdit_CylinderCmd_2->setText("伸出");
        if (unit.value(1)==2) ui->lineEdit_CylinderCmd_2->setText("缩回");

        setGreen(ui->lineEdit_PutterPLState_1,unit.value(2)==1);
        setGreen(ui->lineEdit_PutterNLState_1,unit.value(3)==1);
        if (unit.value(4)==1) ui->lineEdit_PutterAct_1->setText("已缩回");
        if (unit.value(4)==2) ui->lineEdit_PutterAct_1->setText("已伸出");
        if (unit.value(4)==10) ui->lineEdit_PutterAct_1->setText("伸出中");
        if (unit.value(4)==20) ui->lineEdit_PutterAct_1->setText("缩回中");
        if (unit.value(4)==99) ui->lineEdit_PutterAct_1->setText("停止");
        if (unit.value(4)==0) ui->lineEdit_PutterAct_1->setText("空闲");

        setGreen(ui->lineEdit_PutterPLState_2,unit.value(5)==1);
        setGreen(ui->lineEdit_PutterNLState_2,unit.value(6)==1);
        if (unit.value(7)==1) ui->lineEdit_PutterAct_2->setText("已缩回");
        if (unit.value(7)==2) ui->lineEdit_PutterAct_2->setText("已伸出");
        if (unit.value(7)==10) ui->lineEdit_PutterAct_2->setText("伸出中");
        if (unit.value(7)==20) ui->lineEdit_PutterAct_2->setText("缩回中");
        if (unit.value(7)==99) ui->lineEdit_PutterAct_2->setText("停止");
        if (unit.value(7)==0) ui->lineEdit_PutterAct_2->setText("空闲");

        setGreen(ui->lineEdit_PutterPLState_3,unit.value(8)==1);
        setGreen(ui->lineEdit_PutterNLState_3,unit.value(9)==1);
        if (unit.value(10)==1) ui->lineEdit_PutterAct_3->setText("已缩回");
        if (unit.value(10)==2) ui->lineEdit_PutterAct_3->setText("已伸出");
        if (unit.value(10)==10) ui->lineEdit_PutterAct_3->setText("伸出中");
        if (unit.value(10)==20) ui->lineEdit_PutterAct_3->setText("缩回中");
        if (unit.value(10)==99) ui->lineEdit_PutterAct_3->setText("停止");
        if (unit.value(10)==0) ui->lineEdit_PutterAct_3->setText("空闲");


        setGreen(ui->lineEdit_PutterPLState_4,unit.value(11)==1);
        setGreen(ui->lineEdit_PutterNLState_4,unit.value(12)==1);
        if (unit.value(13)==1) ui->lineEdit_PutterAct_4->setText("已缩回");
        if (unit.value(13)==2) ui->lineEdit_PutterAct_4->setText("已伸出");
        if (unit.value(13)==10) ui->lineEdit_PutterAct_4->setText("伸出中");
        if (unit.value(13)==20) ui->lineEdit_PutterAct_4->setText("缩回中");
        if (unit.value(13)==99) ui->lineEdit_PutterAct_4->setText("停止");
        if (unit.value(13)==0) ui->lineEdit_PutterAct_4->setText("空闲");

        setGreen(ui->lineEdit_PutterPLState_5,unit.value(14)==1);
        setGreen(ui->lineEdit_PutterNLState_5,unit.value(15)==1);
        if (unit.value(16)==1) ui->lineEdit_PutterAct_5->setText("已缩回");
        if (unit.value(16)==2) ui->lineEdit_PutterAct_5->setText("已伸出");
        if (unit.value(16)==10) ui->lineEdit_PutterAct_5->setText("伸出中");
        if (unit.value(16)==20) ui->lineEdit_PutterAct_5->setText("缩回中");
        if (unit.value(16)==99) ui->lineEdit_PutterAct_5->setText("停止");
        if (unit.value(16)==0) ui->lineEdit_PutterAct_5->setText("空闲");

        if (unit.value(17)==1) ui->lineEdit_GetPaper->setText("伸出");
        if (unit.value(17)==2) ui->lineEdit_GetPaper->setText("缩回");


        // setGreen(ui->lineEdit_PutterPLState_1,unit.value(22)==1);
        // setGreen(ui->lineEdit_PutterNLState_1,unit.value(23)==1);
        // if (unit.value(24)==1) ui->lineEdit_PutterAct_1->setText("伸出");
        // if (unit.value(24)==2) ui->lineEdit_PutterAct_1->setText("缩回");






        reply->deleteLater();
    };

    connect(reply, &QModbusReply::finished, this, handle);
    if (reply->isFinished()) handle();
}




void Widget::on_btn_Axi_All_Enable_clicked()
{
    const int serverId  = 1;
    const int addr      = 310;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_Axi_All_CloseEnable_clicked()
{

        const int serverId  = 1;
        const int addr      = 310;     // 相对定位标识-轴6
        const quint16 onVal = 2;
        const quint16 offVal= 0;
        const int holdMs    = 100;

        QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
        unitOn.setValue(0, onVal);
        QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
        if (!replyOn) {
            qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
            return;
        }

        connect(replyOn, &QModbusReply::finished, this, [=]() {
            if (replyOn->error() != QModbusDevice::NoError) {
                qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
                replyOn->deleteLater();
                return;
            }
            replyOn->deleteLater();

            QTimer::singleShot(holdMs, this, [=]() {
                QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
                unitOff.setValue(0, offVal);

                QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
                if (!replyOff) {
                    qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                    return;
                }
                connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
            });
        });

}

void Widget::on_btn_Axi_All_Start_clicked()
{
    const int serverId  = 1;
    const int addr      = 311;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}
void Widget::on_btn_Axi_All_Stop_clicked()
{
    const int serverId  = 1;
    const int addr      = 312;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}
void Widget::on_btn_Axi_ALL_Reset_clicked()
{
    const int serverId  = 1;
    const int addr      = 313;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}
void Widget::on_btn_Axi_All_GoHome_clicked()
{
    const int serverId  = 1;
    const int addr      = 314;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}






void Widget::Set_On_Manu()
{
    const int serverId  = 1;
    const int addr      = 315;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::Set_On_Auto()
{
    const int serverId  = 1;
    const int addr      = 315;     // 相对定位标识-轴6
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}
void Widget::on_btn_Axi_Mode_Switch_clicked()
{
    if (ui->lineEdit_Mode->text()=="手动")
    {
         qDebug() << "ui->lineEdit_Mode->text()==\"手动\"";
        Set_On_Auto();
    }
    else
    {
        Set_On_Manu();
    }
}


void Widget::on_ManuButton_clicked(bool checked)
{
    if(checked)
    {
       Set_On_Manu();
    }
}


void Widget::loadParams_Axi_1()
{
    // QSettings s("MyCompany", "MyApp");

    // ui->lineEdit_Standby_Axi_1->setText(
    //     s.value("Params/waitPos_axi1", ui->lineEdit_Standby_Axi_1->text()).toString()
    //     );

    // ui->lineEdit_WorkPostion1_Axi_1->setText(
    //     s.value("Params/workPos1_axi1", ui->lineEdit_WorkPostion1_Axi_1->text()).toString()
    //     );

    // // 下面 2~5 我按你保存时的 key：waitPos2/3/4/5 来读取
    // ui->lineEdit_WorkPostion2_Axi_1->setText(
    //     s.value("Params/waitPos2_axi1", ui->lineEdit_WorkPostion2_Axi_1->text()).toString()
    //     );

    // ui->lineEdit_WorkPostion3_Axi_1->setText(
    //     s.value("Params/waitPos3_axi1", ui->lineEdit_WorkPostion3_Axi_1->text()).toString()
    //     );

    // ui->lineEdit_WorkPostion4_Axi_1->setText(
    //     s.value("Params/waitPos4_axi1", ui->lineEdit_WorkPostion4_Axi_1->text()).toString()
    //     );

    // ui->lineEdit_WorkPostion5_Axi_1->setText(
    //     s.value("Params/waitPos5_axi1", ui->lineEdit_WorkPostion5_Axi_1->text()).toString()
    //     );

    // ui->lineEdit_ManuVel_Axi_1->setText(
    //     s.value("Params/ManuVel_axi1", ui->lineEdit_ManuVel_Axi_1->text()).toString()
    //     );

    // ui->lineEdit_AutoVel_Axi_1->setText(
    //     s.value("Params/AutoVel_axi1", ui->lineEdit_AutoVel_Axi_1->text()).toString()
    //     );

    // ui->lineEdit_Inch_Axi_1->setText(
    //     s.value("Params/Inch_axi1", ui->lineEdit_Inch_Axi_1->text()).toString()
    //     );

    // ui->lineEdit_MoveRel_Axi_1->setText(
    //     s.value("Params/MoveRel_axi1", ui->lineEdit_MoveRel_Axi_1->text()).toString()
    //     );
    // ui->lineEdit_MoveAbs_Axi_1->setText(
    //     s.value("Params/MoveAb_axi1", ui->lineEdit_MoveAbs_Axi_1->text()).toString()
    //     );


        QSettings s("MyCompany", "MyApp");

        // ======================== Axis 1 ========================
        ui->lineEdit_Standby_Axi_1->setText(
            s.value("Params/waitPos_axi1", ui->lineEdit_Standby_Axi_1->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_1->setText(
            s.value("Params/workPos1_axi1", ui->lineEdit_WorkPostion1_Axi_1->text()).toString()
            );

        // 下面 2~5 我按你保存时的 key：waitPos2/3/4/5 来读取
        ui->lineEdit_WorkPostion2_Axi_1->setText(
            s.value("Params/waitPos2_axi1", ui->lineEdit_WorkPostion2_Axi_1->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_1->setText(
            s.value("Params/waitPos3_axi1", ui->lineEdit_WorkPostion3_Axi_1->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_1->setText(
            s.value("Params/waitPos4_axi1", ui->lineEdit_WorkPostion4_Axi_1->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_1->setText(
            s.value("Params/waitPos5_axi1", ui->lineEdit_WorkPostion5_Axi_1->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_1->setText(
            s.value("Params/ManuVel_axi1", ui->lineEdit_ManuVel_Axi_1->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_1->setText(
            s.value("Params/AutoVel_axi1", ui->lineEdit_AutoVel_Axi_1->text()).toString()
            );

        ui->lineEdit_Inch_Axi_1->setText(
            s.value("Params/Inch_axi1", ui->lineEdit_Inch_Axi_1->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_1->setText(
            s.value("Params/MoveRel_axi1", ui->lineEdit_MoveRel_Axi_1->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_1->setText(
            s.value("Params/MoveAb_axi1", ui->lineEdit_MoveAbs_Axi_1->text()).toString()
            );


        // ======================== Axis 2 ========================
        ui->lineEdit_Standby_Axi_2->setText(
            s.value("Params/waitPos_axi2", ui->lineEdit_Standby_Axi_2->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_2->setText(
            s.value("Params/workPos1_axi2", ui->lineEdit_WorkPostion1_Axi_2->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_2->setText(
            s.value("Params/waitPos2_axi2", ui->lineEdit_WorkPostion2_Axi_2->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_2->setText(
            s.value("Params/waitPos3_axi2", ui->lineEdit_WorkPostion3_Axi_2->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_2->setText(
            s.value("Params/waitPos4_axi2", ui->lineEdit_WorkPostion4_Axi_2->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_2->setText(
            s.value("Params/waitPos5_axi2", ui->lineEdit_WorkPostion5_Axi_2->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_2->setText(
            s.value("Params/ManuVel_axi2", ui->lineEdit_ManuVel_Axi_2->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_2->setText(
            s.value("Params/AutoVel_axi2", ui->lineEdit_AutoVel_Axi_2->text()).toString()
            );

        ui->lineEdit_Inch_Axi_2->setText(
            s.value("Params/Inch_axi2", ui->lineEdit_Inch_Axi_2->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_2->setText(
            s.value("Params/MoveRel_axi2", ui->lineEdit_MoveRel_Axi_2->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_2->setText(
            s.value("Params/MoveAb_axi2", ui->lineEdit_MoveAbs_Axi_2->text()).toString()
            );


        // ======================== Axis 3 ========================
        ui->lineEdit_Standby_Axi_3->setText(
            s.value("Params/waitPos_axi3", ui->lineEdit_Standby_Axi_3->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_3->setText(
            s.value("Params/workPos1_axi3", ui->lineEdit_WorkPostion1_Axi_3->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_3->setText(
            s.value("Params/waitPos2_axi3", ui->lineEdit_WorkPostion2_Axi_3->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_3->setText(
            s.value("Params/waitPos3_axi3", ui->lineEdit_WorkPostion3_Axi_3->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_3->setText(
            s.value("Params/waitPos4_axi3", ui->lineEdit_WorkPostion4_Axi_3->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_3->setText(
            s.value("Params/waitPos5_axi3", ui->lineEdit_WorkPostion5_Axi_3->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_3->setText(
            s.value("Params/ManuVel_axi3", ui->lineEdit_ManuVel_Axi_3->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_3->setText(
            s.value("Params/AutoVel_axi3", ui->lineEdit_AutoVel_Axi_3->text()).toString()
            );

        ui->lineEdit_Inch_Axi_3->setText(
            s.value("Params/Inch_axi3", ui->lineEdit_Inch_Axi_3->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_3->setText(
            s.value("Params/MoveRel_axi3", ui->lineEdit_MoveRel_Axi_3->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_3->setText(
            s.value("Params/MoveAb_axi3", ui->lineEdit_MoveAbs_Axi_3->text()).toString()
            );


        // ======================== Axis 4 ========================
        ui->lineEdit_Standby_Axi_4->setText(
            s.value("Params/waitPos_axi4", ui->lineEdit_Standby_Axi_4->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_4->setText(
            s.value("Params/workPos1_axi4", ui->lineEdit_WorkPostion1_Axi_4->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_4->setText(
            s.value("Params/waitPos2_axi4", ui->lineEdit_WorkPostion2_Axi_4->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_4->setText(
            s.value("Params/waitPos3_axi4", ui->lineEdit_WorkPostion3_Axi_4->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_4->setText(
            s.value("Params/waitPos4_axi4", ui->lineEdit_WorkPostion4_Axi_4->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_4->setText(
            s.value("Params/waitPos5_axi4", ui->lineEdit_WorkPostion5_Axi_4->text()).toString()
            );

        ui->lineEdit_WorkPostion6_Axi_4->setText(
            s.value("Params/waitPos6_axi4", ui->lineEdit_WorkPostion6_Axi_4->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_4->setText(
            s.value("Params/ManuVel_axi4", ui->lineEdit_ManuVel_Axi_4->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_4->setText(
            s.value("Params/AutoVel_axi4", ui->lineEdit_AutoVel_Axi_4->text()).toString()
            );

        ui->lineEdit_Inch_Axi_4->setText(
            s.value("Params/Inch_axi4", ui->lineEdit_Inch_Axi_4->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_4->setText(
            s.value("Params/MoveRel_axi4", ui->lineEdit_MoveRel_Axi_4->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_4->setText(
            s.value("Params/MoveAb_axi4", ui->lineEdit_MoveAbs_Axi_4->text()).toString()
            );


        // ======================== Axis 5 ========================
        ui->lineEdit_Standby_Axi_5->setText(
            s.value("Params/waitPos_axi5", ui->lineEdit_Standby_Axi_5->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_5->setText(
            s.value("Params/workPos1_axi5", ui->lineEdit_WorkPostion1_Axi_5->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_5->setText(
            s.value("Params/waitPos2_axi5", ui->lineEdit_WorkPostion2_Axi_5->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_5->setText(
            s.value("Params/waitPos3_axi5", ui->lineEdit_WorkPostion3_Axi_5->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_5->setText(
            s.value("Params/waitPos4_axi5", ui->lineEdit_WorkPostion4_Axi_5->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_5->setText(
            s.value("Params/waitPos5_axi5", ui->lineEdit_WorkPostion5_Axi_5->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_5->setText(
            s.value("Params/ManuVel_axi5", ui->lineEdit_ManuVel_Axi_5->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_5->setText(
            s.value("Params/AutoVel_axi5", ui->lineEdit_AutoVel_Axi_5->text()).toString()
            );

        ui->lineEdit_Inch_Axi_5->setText(
            s.value("Params/Inch_axi5", ui->lineEdit_Inch_Axi_5->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_5->setText(
            s.value("Params/MoveRel_axi5", ui->lineEdit_MoveRel_Axi_5->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_5->setText(
            s.value("Params/MoveAb_axi5", ui->lineEdit_MoveAbs_Axi_5->text()).toString()
            );


        // ======================== Axis 6 ========================
        ui->lineEdit_Standby_Axi_6->setText(
            s.value("Params/waitPos_axi6", ui->lineEdit_Standby_Axi_6->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_6->setText(
            s.value("Params/workPos1_axi6", ui->lineEdit_WorkPostion1_Axi_6->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_6->setText(
            s.value("Params/waitPos2_axi6", ui->lineEdit_WorkPostion2_Axi_6->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_6->setText(
            s.value("Params/waitPos3_axi6", ui->lineEdit_WorkPostion3_Axi_6->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_6->setText(
            s.value("Params/waitPos4_axi6", ui->lineEdit_WorkPostion4_Axi_6->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_6->setText(
            s.value("Params/waitPos5_axi6", ui->lineEdit_WorkPostion5_Axi_6->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_6->setText(
            s.value("Params/ManuVel_axi6", ui->lineEdit_ManuVel_Axi_6->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_6->setText(
            s.value("Params/AutoVel_axi6", ui->lineEdit_AutoVel_Axi_6->text()).toString()
            );

        ui->lineEdit_Inch_Axi_6->setText(
            s.value("Params/Inch_axi6", ui->lineEdit_Inch_Axi_6->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_6->setText(
            s.value("Params/MoveRel_axi6", ui->lineEdit_MoveRel_Axi_6->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_6->setText(
            s.value("Params/MoveAb_axi6", ui->lineEdit_MoveAbs_Axi_6->text()).toString()
            );


        // ======================== Axis 7 ========================
        ui->lineEdit_Standby_Axi_7->setText(
            s.value("Params/waitPos_axi7", ui->lineEdit_Standby_Axi_7->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_7->setText(
            s.value("Params/workPos1_axi7", ui->lineEdit_WorkPostion1_Axi_7->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_7->setText(
            s.value("Params/waitPos2_axi7", ui->lineEdit_WorkPostion2_Axi_7->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_7->setText(
            s.value("Params/waitPos3_axi7", ui->lineEdit_WorkPostion3_Axi_7->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_7->setText(
            s.value("Params/waitPos4_axi7", ui->lineEdit_WorkPostion4_Axi_7->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_7->setText(
            s.value("Params/waitPos5_axi7", ui->lineEdit_WorkPostion5_Axi_7->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_7->setText(
            s.value("Params/ManuVel_axi7", ui->lineEdit_ManuVel_Axi_7->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_7->setText(
            s.value("Params/AutoVel_axi7", ui->lineEdit_AutoVel_Axi_7->text()).toString()
            );

        ui->lineEdit_Inch_Axi_7->setText(
            s.value("Params/Inch_axi7", ui->lineEdit_Inch_Axi_7->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_7->setText(
            s.value("Params/MoveRel_axi7", ui->lineEdit_MoveRel_Axi_7->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_7->setText(
            s.value("Params/MoveAb_axi7", ui->lineEdit_MoveAbs_Axi_7->text()).toString()
            );


        // ======================== Axis 8 ========================
        ui->lineEdit_Standby_Axi_8->setText(
            s.value("Params/waitPos_axi8", ui->lineEdit_Standby_Axi_8->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_8->setText(
            s.value("Params/workPos1_axi8", ui->lineEdit_WorkPostion1_Axi_8->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_8->setText(
            s.value("Params/waitPos2_axi8", ui->lineEdit_WorkPostion2_Axi_8->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_8->setText(
            s.value("Params/waitPos3_axi8", ui->lineEdit_WorkPostion3_Axi_8->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_8->setText(
            s.value("Params/waitPos4_axi8", ui->lineEdit_WorkPostion4_Axi_8->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_8->setText(
            s.value("Params/waitPos5_axi8", ui->lineEdit_WorkPostion5_Axi_8->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_8->setText(
            s.value("Params/ManuVel_axi8", ui->lineEdit_ManuVel_Axi_8->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_8->setText(
            s.value("Params/AutoVel_axi8", ui->lineEdit_AutoVel_Axi_8->text()).toString()
            );

        ui->lineEdit_Inch_Axi_8->setText(
            s.value("Params/Inch_axi8", ui->lineEdit_Inch_Axi_8->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_8->setText(
            s.value("Params/MoveRel_axi8", ui->lineEdit_MoveRel_Axi_8->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_8->setText(
            s.value("Params/MoveAb_axi8", ui->lineEdit_MoveAbs_Axi_8->text()).toString()
            );


        // ======================== Axis 9 ========================
        ui->lineEdit_Standby_Axi_9->setText(
            s.value("Params/waitPos_axi9", ui->lineEdit_Standby_Axi_9->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_9->setText(
            s.value("Params/workPos1_axi9", ui->lineEdit_WorkPostion1_Axi_9->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_9->setText(
            s.value("Params/waitPos2_axi9", ui->lineEdit_WorkPostion2_Axi_9->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_9->setText(
            s.value("Params/waitPos3_axi9", ui->lineEdit_WorkPostion3_Axi_9->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_9->setText(
            s.value("Params/waitPos4_axi9", ui->lineEdit_WorkPostion4_Axi_9->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_9->setText(
            s.value("Params/waitPos5_axi9", ui->lineEdit_WorkPostion5_Axi_9->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_9->setText(
            s.value("Params/ManuVel_axi9", ui->lineEdit_ManuVel_Axi_9->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_9->setText(
            s.value("Params/AutoVel_axi9", ui->lineEdit_AutoVel_Axi_9->text()).toString()
            );

        ui->lineEdit_Inch_Axi_9->setText(
            s.value("Params/Inch_axi9", ui->lineEdit_Inch_Axi_9->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_9->setText(
            s.value("Params/MoveRel_axi9", ui->lineEdit_MoveRel_Axi_9->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_9->setText(
            s.value("Params/MoveAb_axi9", ui->lineEdit_MoveAbs_Axi_9->text()).toString()
            );


        // ======================== Axis 10 ========================
        ui->lineEdit_Standby_Axi_10->setText(
            s.value("Params/waitPos_axi10", ui->lineEdit_Standby_Axi_10->text()).toString()
            );

        ui->lineEdit_WorkPostion1_Axi_10->setText(
            s.value("Params/workPos1_axi10", ui->lineEdit_WorkPostion1_Axi_10->text()).toString()
            );

        ui->lineEdit_WorkPostion2_Axi_10->setText(
            s.value("Params/waitPos2_axi10", ui->lineEdit_WorkPostion2_Axi_10->text()).toString()
            );

        ui->lineEdit_WorkPostion3_Axi_10->setText(
            s.value("Params/waitPos3_axi10", ui->lineEdit_WorkPostion3_Axi_10->text()).toString()
            );

        ui->lineEdit_WorkPostion4_Axi_10->setText(
            s.value("Params/waitPos4_axi10", ui->lineEdit_WorkPostion4_Axi_10->text()).toString()
            );

        ui->lineEdit_WorkPostion5_Axi_10->setText(
            s.value("Params/waitPos5_axi10", ui->lineEdit_WorkPostion5_Axi_10->text()).toString()
            );

        ui->lineEdit_ManuVel_Axi_10->setText(
            s.value("Params/ManuVel_axi10", ui->lineEdit_ManuVel_Axi_10->text()).toString()
            );

        ui->lineEdit_AutoVel_Axi_10->setText(
            s.value("Params/AutoVel_axi10", ui->lineEdit_AutoVel_Axi_10->text()).toString()
            );

        ui->lineEdit_Inch_Axi_10->setText(
            s.value("Params/Inch_axi10", ui->lineEdit_Inch_Axi_10->text()).toString()
            );

        ui->lineEdit_MoveRel_Axi_10->setText(
            s.value("Params/MoveRel_axi10", ui->lineEdit_MoveRel_Axi_10->text()).toString()
            );
        ui->lineEdit_MoveAbs_Axi_10->setText(
            s.value("Params/MoveAb_axi10", ui->lineEdit_MoveAbs_Axi_10->text()).toString()
            );


}

void Widget::on_AutoButton_clicked(bool checked)
{
    if(checked)
    {
        Set_On_Auto();
    }
    else
    {
        Set_On_Manu();
    }
}




void Widget::on_btn_Enable_Open_Axi_1_clicked()
{
    const int serverId   = 1;
    const int addr       = 15;     // 触发寄存器
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;    // 脉冲保持时间（50~200ms）

    // 1) 点击后先禁用按钮，防止连点
    ui->btn_Enable_Open_Axi_1->setEnabled(false);

    // 2) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request.";
        ui->btn_Enable_Open_Axi_1->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        // 第一步完成先检查错误
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_1->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        // 3) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request.";
                ui->btn_Enable_Open_Axi_1->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed:" << replyOff->errorString();
                }
                replyOff->deleteLater();

                // 4) 第二步完成后恢复按钮（成功失败都恢复）
                ui->btn_Enable_Open_Axi_1->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 15;     // 触发寄存器
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_1_pressed()
{
    const int serverId = 1;
    const int addrBase = 16;  // 你这段里是从17开始写6个
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal); // D22(假设)=Jog_Pos=1

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Nes(1)";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}


void Widget::on_btn_Jog_Neg_Axi_1_released()
{
    const int serverId = 1;
    const int addr     = 16;      // = 17 + 5，你原来 off 写的是 addr+5
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0)";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });

}




void Widget::on_btn_Jog_Pos_Axi_1_pressed()
{

    const int serverId = 1;
    const int addrBase = 17;  // 你这段里是从17开始写6个
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal); // D22(假设)=Jog_Pos=1

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1)";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });


}



void Widget::on_btn_Jog_Pos_Axi_1_released()
{

    const int serverId = 1;
    const int addr     = 17;      // = 17 + 5，你原来 off 写的是 addr+5
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0)";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });


}




void Widget::on_btn_Stop_Axi_1_clicked()
{

    const int serverId  = 1;
    const int addr      = 18;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop_Axi(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop_Axi(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop_Axi(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_1_clicked()
{

    const int serverId  = 1;
    const int addr      = 19;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset_Axi(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset_Axi(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset_Axi(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_GoHome_Axi_1_clicked()
{

    const int serverId  = 1;
    const int addr      = 20;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);

    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to GoHome reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_MoveAbs_Axi_1_clicked()
{


    const int serverId  = 1;
    const int addr      = 21;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to MoveAbs reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });


}


void Widget::on_btn_MoveRel_Axi_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 22;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to MoveRel reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_SetHome_Axi_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 23;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetHome reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}





void Widget::on_btn_Jog_SoftOpen_Axi_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 24;     // 触发寄存器
    const quint16 onVal = 0;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Jog_SoftOpen(1) request.";
        return;
    }

}


void Widget::on_btn_Jog_SoftClose_Axi_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 24;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose(1) request.";
        return;
    }
}
void Widget::on_btn_Jog_Jog_Axi_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 25;     // 触发寄存器
    const quint16 onVal = 0;
    const quint16 offVal= 1;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Jog_Jog(1) request.";
        return;
    }
}


void Widget::on_btn_Jog_Inch_Axi_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 25;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Jog_Inch(1) request.";
        return;
    }
}



// ======================== Axis 2 (轴2) ========================
void Widget::on_btn_Enable_Open_Axi_2_clicked()
{
    const int serverId   = 1;
    const int addr       = 45;     // 使能开-轴2（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_2->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_2).";
        ui->btn_Enable_Open_Axi_2->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_2):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_2->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_2).";
                ui->btn_Enable_Open_Axi_2->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_2):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_2->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 45;     // 使能开-轴2（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_2).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_2):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_2).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_2_pressed()
{
    const int serverId = 1;
    const int addrBase = 46;     // 负向点动-轴2
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_2).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_2_released()
{
    const int serverId = 1;
    const int addr     = 46;     // 负向点动-轴2
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_2).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_2_pressed()
{
    const int serverId = 1;
    const int addrBase = 47;     // 正向点动-轴2
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_2).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_2_released()
{
    const int serverId = 1;
    const int addr     = 47;     // 正向点动-轴2
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_2).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 48;     // 停止-轴2
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_2).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_2):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_2).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 49;     // 复位-轴2
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_2).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_2):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_2).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 50;     // 回原点-轴2
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_2).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_2):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_2).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 51;     // 绝对定位标识-轴2
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_2).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_2):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_2).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 52;     // 相对定位标识-轴2
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_2).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_2):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_2).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 53;     // 设为原点-轴2
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_2).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_2):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_2).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 54;     // 软限位使能-轴2（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_2).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 54;     // 软限位使能-轴2
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_2).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 55;     // 点动寸动模式-轴2（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_2).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 55;     // 点动寸动模式-轴2（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_2).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

// ======================== Axis 3 (轴3) ========================
void Widget::on_btn_Enable_Open_Axi_3_clicked()
{
    const int serverId   = 1;
    const int addr       = 75;     // 使能开-轴3（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_3->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_3).";
        ui->btn_Enable_Open_Axi_3->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_3):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_3->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_3).";
                ui->btn_Enable_Open_Axi_3->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_3):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_3->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 75;     // 使能开-轴3（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_3).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_3):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_3).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_3_pressed()
{
    const int serverId = 1;
    const int addrBase = 76;     // 负向点动-轴3
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_3).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_3_released()
{
    const int serverId = 1;
    const int addr     = 76;     // 负向点动-轴3
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_3).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_3_pressed()
{
    const int serverId = 1;
    const int addrBase = 77;     // 正向点动-轴3
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_3).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_3_released()
{
    const int serverId = 1;
    const int addr     = 77;     // 正向点动-轴3
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_3).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 78;     // 停止-轴3
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_3).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_3):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_3).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 79;     // 复位-轴3
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_3).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_3):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_3).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 80;     // 回原点-轴3
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_3).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_3):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_3).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 81;     // 绝对定位标识-轴3
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_3).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_3):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_3).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 82;     // 相对定位标识-轴3
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_3).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_3):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_3).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 83;     // 设为原点-轴3
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_3).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_3):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_3).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 84;     // 软限位使能-轴3（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_3).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 84;     // 软限位使能-轴3
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_3).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 85;     // 点动寸动模式-轴3（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_3).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 85;     // 点动寸动模式-轴3（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_3).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

// ======================== Axis 4 (轴4) ========================
void Widget::on_btn_Enable_Open_Axi_4_clicked()
{
    const int serverId   = 1;
    const int addr       = 105;     // 使能开-轴4（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_4->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_4).";
        ui->btn_Enable_Open_Axi_4->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_4):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_4->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_4).";
                ui->btn_Enable_Open_Axi_4->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_4):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_4->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 105;     // 使能开-轴4（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_4).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_4):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_4).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_4_pressed()
{
    const int serverId = 1;
    const int addrBase = 106;     // 负向点动-轴4
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_4).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_4_released()
{
    const int serverId = 1;
    const int addr     = 106;     // 负向点动-轴4
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_4).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_4_pressed()
{
    const int serverId = 1;
    const int addrBase = 107;     // 正向点动-轴4
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_4).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_4_released()
{
    const int serverId = 1;
    const int addr     = 107;     // 正向点动-轴4
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_4).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 108;     // 停止-轴4
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_4).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_4):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_4).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 109;     // 复位-轴4
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_4).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_4):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_4).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 110;     // 回原点-轴4
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_4).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_4):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_4).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 111;     // 绝对定位标识-轴4
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_4).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_4):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_4).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 112;     // 相对定位标识-轴4
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_4).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_4):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_4).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 113;     // 设为原点-轴4
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_4).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_4):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_4).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 114;     // 软限位使能-轴4（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_4).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 114;     // 软限位使能-轴4
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_4).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 115;     // 点动寸动模式-轴4（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_4).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 115;     // 点动寸动模式-轴4（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_4).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

// ======================== Axis 5 (轴5) ========================
void Widget::on_btn_Enable_Open_Axi_5_clicked()
{
    const int serverId   = 1;
    const int addr       = 135;     // 使能开-轴5（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_5->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_5).";
        ui->btn_Enable_Open_Axi_5->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_5):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_5->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_5).";
                ui->btn_Enable_Open_Axi_5->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_5):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_5->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 135;     // 使能开-轴5（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_5).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_5):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_5).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_5_pressed()
{
    const int serverId = 1;
    const int addrBase = 136;     // 负向点动-轴5
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_5).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_5_released()
{
    const int serverId = 1;
    const int addr     = 136;     // 负向点动-轴5
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_5).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_5_pressed()
{
    const int serverId = 1;
    const int addrBase = 137;     // 正向点动-轴5
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_5).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_5_released()
{
    const int serverId = 1;
    const int addr     = 137;     // 正向点动-轴5
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_5).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 138;     // 停止-轴5
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_5).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_5):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_5).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 139;     // 复位-轴5
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_5).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_5):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_5).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 140;     // 回原点-轴5
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_5).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_5):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_5).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 141;     // 绝对定位标识-轴5
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_5).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_5):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_5).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 142;     // 相对定位标识-轴5
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_5).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_5):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_5).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 143;     // 设为原点-轴5
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_5).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_5):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_5).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 144;     // 软限位使能-轴5（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_5).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 144;     // 软限位使能-轴5
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_5).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 145;     // 点动寸动模式-轴5（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_5).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 145;     // 点动寸动模式-轴5（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_5).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

// ======================== Axis 6 (轴6) ========================
void Widget::on_btn_Enable_Open_Axi_6_clicked()
{
    const int serverId   = 1;
    const int addr       = 165;     // 使能开-轴6（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_6->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_6).";
        ui->btn_Enable_Open_Axi_6->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_6->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_6).";
                ui->btn_Enable_Open_Axi_6->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_6):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_6->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 165;     // 使能开-轴6（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_6_pressed()
{
    const int serverId = 1;
    const int addrBase = 166;     // 负向点动-轴6
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_6).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_6_released()
{
    const int serverId = 1;
    const int addr     = 166;     // 负向点动-轴6
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_6).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_6_pressed()
{
    const int serverId = 1;
    const int addrBase = 167;     // 正向点动-轴6
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_6).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_6_released()
{
    const int serverId = 1;
    const int addr     = 167;     // 正向点动-轴6
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_6).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 168;     // 停止-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 169;     // 复位-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 170;     // 回原点-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 171;     // 绝对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 172;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 173;     // 设为原点-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 174;     // 软限位使能-轴6（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_6).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 174;     // 软限位使能-轴6
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_6).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 175;     // 点动寸动模式-轴6（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_6).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 175;     // 点动寸动模式-轴6（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_6).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

// ======================== Axis 7 (轴7) ========================
void Widget::on_btn_Enable_Open_Axi_7_clicked()
{
    const int serverId   = 1;
    const int addr       = 195;     // 使能开-轴7（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_7->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_7).";
        ui->btn_Enable_Open_Axi_7->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_7):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_7->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_7).";
                ui->btn_Enable_Open_Axi_7->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_7):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_7->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 195;     // 使能开-轴7（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_7).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_7):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_7).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_7_pressed()
{
    const int serverId = 1;
    const int addrBase = 196;     // 负向点动-轴7
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_7).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_7_released()
{
    const int serverId = 1;
    const int addr     = 196;     // 负向点动-轴7
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_7).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_7_pressed()
{
    const int serverId = 1;
    const int addrBase = 197;     // 正向点动-轴7
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_7).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_7_released()
{
    const int serverId = 1;
    const int addr     = 197;     // 正向点动-轴7
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_7).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 198;     // 停止-轴7
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_7).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_7):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_7).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 199;     // 复位-轴7
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_7).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_7):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_7).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 200;     // 回原点-轴7
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_7).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_7):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_7).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 201;     // 绝对定位标识-轴7
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_7).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_7):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_7).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 202;     // 相对定位标识-轴7
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_7).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_7):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_7).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 203;     // 设为原点-轴7
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_7).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_7):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_7).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 204;     // 软限位使能-轴7（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_7).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 204;     // 软限位使能-轴7
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_7).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 205;     // 点动寸动模式-轴7（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_7).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 205;     // 点动寸动模式-轴7（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_7).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

// ======================== Axis 8 (轴8) ========================
void Widget::on_btn_Enable_Open_Axi_8_clicked()
{
    const int serverId   = 1;
    const int addr       = 225;     // 使能开-轴8（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_8->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_8).";
        ui->btn_Enable_Open_Axi_8->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_8):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_8->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_8).";
                ui->btn_Enable_Open_Axi_8->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_8):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_8->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 225;     // 使能开-轴8（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_8).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_8):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_8).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_8_pressed()
{
    const int serverId = 1;
    const int addrBase = 226;     // 负向点动-轴8
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_8).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_8_released()
{
    const int serverId = 1;
    const int addr     = 226;     // 负向点动-轴8
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_8).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_8_pressed()
{
    const int serverId = 1;
    const int addrBase = 227;     // 正向点动-轴8
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_8).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_8_released()
{
    const int serverId = 1;
    const int addr     = 227;     // 正向点动-轴8
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_8).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 228;     // 停止-轴8
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_8).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_8):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_8).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 229;     // 复位-轴8
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_8).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_8):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_8).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 230;     // 回原点-轴8
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_8).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_8):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_8).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 231;     // 绝对定位标识-轴8
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_8).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_8):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_8).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 232;     // 相对定位标识-轴8
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_8).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_8):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_8).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 233;     // 设为原点-轴8
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_8).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_8):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_8).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 234;     // 软限位使能-轴8（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_8).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 234;     // 软限位使能-轴8
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_8).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 235;     // 点动寸动模式-轴8（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_8).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 235;     // 点动寸动模式-轴8（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_8).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

// ======================== Axis 9 (轴9) ========================
void Widget::on_btn_Enable_Open_Axi_9_clicked()
{
    const int serverId   = 1;
    const int addr       = 255;     // 使能开-轴9（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_9->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_9).";
        ui->btn_Enable_Open_Axi_9->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_9):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_9->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_9).";
                ui->btn_Enable_Open_Axi_9->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_9):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_9->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 255;     // 使能开-轴9（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_9).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_9):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_9).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_9_pressed()
{
    const int serverId = 1;
    const int addrBase = 256;     // 负向点动-轴9
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_9).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_9_released()
{
    const int serverId = 1;
    const int addr     = 256;     // 负向点动-轴9
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_9).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_9_pressed()
{
    const int serverId = 1;
    const int addrBase = 257;     // 正向点动-轴9
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_9).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_9_released()
{
    const int serverId = 1;
    const int addr     = 257;     // 正向点动-轴9
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_9).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 258;     // 停止-轴9
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_9).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_9):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_9).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 259;     // 复位-轴9
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_9).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_9):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_9).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 260;     // 回原点-轴9
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_9).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_9):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_9).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 261;     // 绝对定位标识-轴9
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_9).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_9):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_9).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 262;     // 相对定位标识-轴9
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_9).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_9):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_9).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 263;     // 设为原点-轴9
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_9).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_9):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_9).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 264;     // 软限位使能-轴9（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_9).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 264;     // 软限位使能-轴9
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_9).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 265;     // 点动寸动模式-轴9（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_9).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_9_clicked()
{
    const int serverId  = 1;
    const int addr      = 265;     // 点动寸动模式-轴9（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_9).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

// ======================== Axis 10 (轴10) ========================
void Widget::on_btn_Enable_Open_Axi_10_clicked()
{
    const int serverId   = 1;
    const int addr       = 285;     // 使能开-轴10（写1开/写2关，随后写0复位）
    const quint16 onVal  = 1;
    const quint16 offVal = 0;
    const int holdMs     = 50;

    ui->btn_Enable_Open_Axi_10->setEnabled(false);

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Open(1) request (Axi_10).";
        ui->btn_Enable_Open_Axi_10->setEnabled(true);
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Open(1) failed (Axi_10):" << replyOn->errorString();
            replyOn->deleteLater();
            ui->btn_Enable_Open_Axi_10->setEnabled(true);
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Open(0) request (Axi_10).";
                ui->btn_Enable_Open_Axi_10->setEnabled(true);
                return;
            }

            connect(replyOff, &QModbusReply::finished, this, [=]() {
                if (replyOff->error() != QModbusDevice::NoError) {
                    qDebug() << "Enable_Open(0) failed (Axi_10):" << replyOff->errorString();
                }
                replyOff->deleteLater();
                ui->btn_Enable_Open_Axi_10->setEnabled(true);
            });
        });
    });
}

void Widget::on_btn_Enable_Close_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 285;     // 使能开-轴10（写1开/写2关，随后写0复位）
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Enable_Close(2) request (Axi_10).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Enable_Close(2) failed (Axi_10):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Enable_Close(0) request (Axi_10).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_Neg_Axi_10_pressed()
{
    const int serverId = 1;
    const int addrBase = 286;     // 负向点动-轴10
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(1) (Axi_10).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Neg_Axi_10_released()
{
    const int serverId = 1;
    const int addr     = 286;     // 负向点动-轴10
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Neg(0) (Axi_10).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Neg(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_10_pressed()
{
    const int serverId = 1;
    const int addrBase = 287;     // 正向点动-轴10
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addrBase, 1);
    unitOn.setValue(0, onVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(1) (Axi_10).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(1) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Jog_Pos_Axi_10_released()
{
    const int serverId = 1;
    const int addr     = 287;     // 正向点动-轴10
    const quint16 offVal = 0;

    QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOff.setValue(0, offVal);

    QModbusReply *reply = modbusClient->sendWriteRequest(unitOff, serverId);
    if (!reply) {
        qDebug() << "Failed to send Jog_Pos(0) (Axi_10).";
        return;
    }

    connect(reply, &QModbusReply::finished, reply, [reply]() {
        if (reply->error() != QModbusDevice::NoError)
            qDebug() << "Jog_Pos(0) failed:" << reply->errorString();
        reply->deleteLater();
    });
}

void Widget::on_btn_Stop_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 288;     // 停止-轴10
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Stop(1) request (Axi_10).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Stop(1) failed (Axi_10):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Stop(0) request (Axi_10).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Reset_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 289;     // 复位-轴10
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Reset(1) request (Axi_10).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Reset(1) failed (Axi_10):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Reset(0) request (Axi_10).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_GoHome_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 290;     // 回原点-轴10
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send GoHome(1) request (Axi_10).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "GoHome(1) failed (Axi_10):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send GoHome(0) request (Axi_10).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveAbs_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 291;     // 绝对定位标识-轴10
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveAbs(1) request (Axi_10).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveAbs(1) failed (Axi_10):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveAbs(0) request (Axi_10).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_MoveRel_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 292;     // 相对定位标识-轴10
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_10).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_10):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_10).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_SetHome_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 293;     // 设为原点-轴10
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetHome(1) request (Axi_10).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetHome(1) failed (Axi_10):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send SetHome(0) request (Axi_10).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Jog_SoftOpen_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 294;     // 软限位使能-轴10（PLC里 bSoftEnable := NOT INT_TO_BOOL(Dxx)）
    const quint16 onVal = 0;   // 0 -> NOT 0 = 1 (软限位开)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_10).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_SoftClose_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 294;     // 软限位使能-轴10
    const quint16 onVal = 1;   // 1 -> NOT 1 = 0 (软限位关)

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftClose request (Axi_10).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Jog_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 295;     // 点动寸动模式-轴10（0=连续点动，1=寸动点动）
    const quint16 onVal = 0;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Jog request (Axi_10).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}

void Widget::on_btn_Jog_Inch_Axi_10_clicked()
{
    const int serverId  = 1;
    const int addr      = 295;     // 点动寸动模式-轴10（0=连续点动，1=寸动点动）
    const quint16 onVal = 1;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send JogMode=Inch request (Axi_10).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}


void Widget::on_btn_SetParam_Axi_1_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi1",ui->lineEdit_Standby_Axi_1->text());

    ui->lineEdit_WorkPostion1_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi1",ui->lineEdit_WorkPostion1_Axi_1->text());

    ui->lineEdit_WorkPostion2_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi1",ui->lineEdit_WorkPostion2_Axi_1->text());

    ui->lineEdit_WorkPostion3_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi1",ui->lineEdit_WorkPostion3_Axi_1->text());

    ui->lineEdit_WorkPostion4_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi1",ui->lineEdit_WorkPostion4_Axi_1->text());

    ui->lineEdit_WorkPostion5_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi1",ui->lineEdit_WorkPostion5_Axi_1->text());

    ui->lineEdit_ManuVel_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi1",ui->lineEdit_ManuVel_Axi_1->text());

    ui->lineEdit_AutoVel_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi1",ui->lineEdit_AutoVel_Axi_1->text());

    ui->lineEdit_Inch_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi1",ui->lineEdit_Inch_Axi_1->text());

    ui->lineEdit_MoveRel_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi1",ui->lineEdit_MoveRel_Axi_1->text());

    ui->lineEdit_MoveAbs_Axi_1->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi1",ui->lineEdit_MoveAbs_Axi_1->text());

    s.sync();



    const int serverId  = 1;
    const int addr      = 0;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_1->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_1->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_1->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_1->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_1->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_1->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_1->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_1->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_1->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_1->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_1->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_1->text().toUShort());


    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::onWriteParamFinished_Axi_1()
{
    auto reply = qobject_cast<QModbusReply*>(sender());
    if (!reply) return;

    // 先检查这次写有没有报错
    if (reply->error() != QModbusDevice::NoError) {
        qDebug() << "Write failed:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    reply->deleteLater();

    QModbusDataUnit resetUnit(QModbusDataUnit::HoldingRegisters, 0, 1);
    resetUnit.setValue(0, 0);

    auto resetReply = modbusClient->sendWriteRequest(resetUnit, 1);
    if (resetReply) {
        connect(resetReply, &QModbusReply::finished, resetReply, &QObject::deleteLater);
    } else {
        qDebug() << "Failed to send reset(0) request.";
    }
}











// ======================== Axis 2 ========================
void Widget::on_btn_SetParam_Axi_2_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi2",ui->lineEdit_Standby_Axi_2->text());

    ui->lineEdit_WorkPostion1_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi2",ui->lineEdit_WorkPostion1_Axi_2->text());

    ui->lineEdit_WorkPostion2_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi2",ui->lineEdit_WorkPostion2_Axi_2->text());

    ui->lineEdit_WorkPostion3_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi2",ui->lineEdit_WorkPostion3_Axi_2->text());

    ui->lineEdit_WorkPostion4_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi2",ui->lineEdit_WorkPostion4_Axi_2->text());

    ui->lineEdit_WorkPostion5_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi2",ui->lineEdit_WorkPostion5_Axi_2->text());

    ui->lineEdit_ManuVel_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi2",ui->lineEdit_ManuVel_Axi_2->text());

    ui->lineEdit_AutoVel_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi2",ui->lineEdit_AutoVel_Axi_2->text());

    ui->lineEdit_Inch_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi2",ui->lineEdit_Inch_Axi_2->text());

    ui->lineEdit_MoveRel_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi2",ui->lineEdit_MoveRel_Axi_2->text());

    ui->lineEdit_MoveAbs_Axi_2->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi2",ui->lineEdit_MoveAbs_Axi_2->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 30;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_2->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_2->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_2->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_2->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_2->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_2->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_2->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_2->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_2->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_2->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_2->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_2->text().toUShort());
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

// ======================== Axis 3 ========================
void Widget::on_btn_SetParam_Axi_3_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi3",ui->lineEdit_Standby_Axi_3->text());

    ui->lineEdit_WorkPostion1_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi3",ui->lineEdit_WorkPostion1_Axi_3->text());

    ui->lineEdit_WorkPostion2_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi3",ui->lineEdit_WorkPostion2_Axi_3->text());

    ui->lineEdit_WorkPostion3_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi3",ui->lineEdit_WorkPostion3_Axi_3->text());

    ui->lineEdit_WorkPostion4_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi3",ui->lineEdit_WorkPostion4_Axi_3->text());

    ui->lineEdit_WorkPostion5_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi3",ui->lineEdit_WorkPostion5_Axi_3->text());

    ui->lineEdit_ManuVel_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi3",ui->lineEdit_ManuVel_Axi_3->text());

    ui->lineEdit_AutoVel_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi3",ui->lineEdit_AutoVel_Axi_3->text());

    ui->lineEdit_Inch_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi3",ui->lineEdit_Inch_Axi_3->text());

    ui->lineEdit_MoveRel_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi3",ui->lineEdit_MoveRel_Axi_3->text());

    ui->lineEdit_MoveAbs_Axi_3->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi3",ui->lineEdit_MoveAbs_Axi_3->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 60;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_3->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_3->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_3->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_3->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_3->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_3->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_3->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_3->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_3->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_3->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_3->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_3->text().toUShort());
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

// ======================== Axis 4 ========================
void Widget::on_btn_SetParam_Axi_4_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi4",ui->lineEdit_Standby_Axi_4->text());

    ui->lineEdit_WorkPostion1_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi4",ui->lineEdit_WorkPostion1_Axi_4->text());

    ui->lineEdit_WorkPostion2_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi4",ui->lineEdit_WorkPostion2_Axi_4->text());

    ui->lineEdit_WorkPostion3_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi4",ui->lineEdit_WorkPostion3_Axi_4->text());

    ui->lineEdit_WorkPostion4_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi4",ui->lineEdit_WorkPostion4_Axi_4->text());

    ui->lineEdit_WorkPostion5_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi4",ui->lineEdit_WorkPostion5_Axi_4->text());

    ui->lineEdit_ManuVel_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi4",ui->lineEdit_ManuVel_Axi_4->text());

    ui->lineEdit_AutoVel_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi4",ui->lineEdit_AutoVel_Axi_4->text());

    ui->lineEdit_Inch_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi4",ui->lineEdit_Inch_Axi_4->text());

    ui->lineEdit_MoveRel_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi4",ui->lineEdit_MoveRel_Axi_4->text());

    ui->lineEdit_MoveAbs_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi4",ui->lineEdit_MoveAbs_Axi_4->text());

    ui->lineEdit_WorkPostion6_Axi_4->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos6_axi4",ui->lineEdit_WorkPostion6_Axi_4->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 90;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 14);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_4->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_4->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_4->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_4->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_4->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_4->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_4->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_4->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_4->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_4->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_4->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_4->text().toUShort());
    unitOn.setValue(13, ui->lineEdit_WorkPostion6_Axi_4->text().toUShort());

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

// ======================== Axis 5 ========================
void Widget::on_btn_SetParam_Axi_5_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi5",ui->lineEdit_Standby_Axi_5->text());

    ui->lineEdit_WorkPostion1_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi5",ui->lineEdit_WorkPostion1_Axi_5->text());

    ui->lineEdit_WorkPostion2_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi5",ui->lineEdit_WorkPostion2_Axi_5->text());

    ui->lineEdit_WorkPostion3_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi5",ui->lineEdit_WorkPostion3_Axi_5->text());

    ui->lineEdit_WorkPostion4_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi5",ui->lineEdit_WorkPostion4_Axi_5->text());

    ui->lineEdit_WorkPostion5_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi5",ui->lineEdit_WorkPostion5_Axi_5->text());

    ui->lineEdit_ManuVel_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi5",ui->lineEdit_ManuVel_Axi_5->text());

    ui->lineEdit_AutoVel_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi5",ui->lineEdit_AutoVel_Axi_5->text());

    ui->lineEdit_Inch_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi5",ui->lineEdit_Inch_Axi_5->text());

    ui->lineEdit_MoveRel_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi5",ui->lineEdit_MoveRel_Axi_5->text());

    ui->lineEdit_MoveAbs_Axi_5->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi5",ui->lineEdit_MoveAbs_Axi_5->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 120;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_5->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_5->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_5->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_5->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_5->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_5->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_5->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_5->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_5->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_5->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_5->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_5->text().toUShort());
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

// ======================== Axis 6 ========================
void Widget::on_btn_SetParam_Axi_6_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi6",ui->lineEdit_Standby_Axi_6->text());

    ui->lineEdit_WorkPostion1_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi6",ui->lineEdit_WorkPostion1_Axi_6->text());

    ui->lineEdit_WorkPostion2_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi6",ui->lineEdit_WorkPostion2_Axi_6->text());

    ui->lineEdit_WorkPostion3_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi6",ui->lineEdit_WorkPostion3_Axi_6->text());

    ui->lineEdit_WorkPostion4_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi6",ui->lineEdit_WorkPostion4_Axi_6->text());

    ui->lineEdit_WorkPostion5_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi6",ui->lineEdit_WorkPostion5_Axi_6->text());

    ui->lineEdit_ManuVel_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi6",ui->lineEdit_ManuVel_Axi_6->text());

    ui->lineEdit_AutoVel_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi6",ui->lineEdit_AutoVel_Axi_6->text());

    ui->lineEdit_Inch_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi6",ui->lineEdit_Inch_Axi_6->text());

    ui->lineEdit_MoveRel_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi6",ui->lineEdit_MoveRel_Axi_6->text());

    ui->lineEdit_MoveAbs_Axi_6->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi6",ui->lineEdit_MoveAbs_Axi_6->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 150;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_6->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_6->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_6->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_6->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_6->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_6->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_6->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_6->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_6->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_6->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_6->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_6->text().toUShort());

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

// ======================== Axis 7 ========================
void Widget::on_btn_SetParam_Axi_7_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi7",ui->lineEdit_Standby_Axi_7->text());

    ui->lineEdit_WorkPostion1_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi7",ui->lineEdit_WorkPostion1_Axi_7->text());

    ui->lineEdit_WorkPostion2_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi7",ui->lineEdit_WorkPostion2_Axi_7->text());

    ui->lineEdit_WorkPostion3_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi7",ui->lineEdit_WorkPostion3_Axi_7->text());

    ui->lineEdit_WorkPostion4_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi7",ui->lineEdit_WorkPostion4_Axi_7->text());

    ui->lineEdit_WorkPostion5_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi7",ui->lineEdit_WorkPostion5_Axi_7->text());

    ui->lineEdit_ManuVel_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi7",ui->lineEdit_ManuVel_Axi_7->text());

    ui->lineEdit_AutoVel_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi7",ui->lineEdit_AutoVel_Axi_7->text());

    ui->lineEdit_Inch_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi7",ui->lineEdit_Inch_Axi_7->text());

    ui->lineEdit_MoveRel_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi7",ui->lineEdit_MoveRel_Axi_7->text());

    ui->lineEdit_MoveAbs_Axi_7->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi7",ui->lineEdit_MoveAbs_Axi_7->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 180;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_7->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_7->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_7->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_7->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_7->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_7->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_7->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_7->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_7->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_7->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_7->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_7->text().toUShort());
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

// ======================== Axis 8 ========================
void Widget::on_btn_SetParam_Axi_8_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi8",ui->lineEdit_Standby_Axi_8->text());

    ui->lineEdit_WorkPostion1_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi8",ui->lineEdit_WorkPostion1_Axi_8->text());

    ui->lineEdit_WorkPostion2_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi8",ui->lineEdit_WorkPostion2_Axi_8->text());

    ui->lineEdit_WorkPostion3_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi8",ui->lineEdit_WorkPostion3_Axi_8->text());

    ui->lineEdit_WorkPostion4_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi8",ui->lineEdit_WorkPostion4_Axi_8->text());

    ui->lineEdit_WorkPostion5_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi8",ui->lineEdit_WorkPostion5_Axi_8->text());

    ui->lineEdit_ManuVel_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi8",ui->lineEdit_ManuVel_Axi_8->text());

    ui->lineEdit_AutoVel_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi8",ui->lineEdit_AutoVel_Axi_8->text());

    ui->lineEdit_Inch_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi8",ui->lineEdit_Inch_Axi_8->text());

    ui->lineEdit_MoveRel_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi8",ui->lineEdit_MoveRel_Axi_8->text());

    ui->lineEdit_MoveAbs_Axi_8->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi8",ui->lineEdit_MoveAbs_Axi_8->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 210;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_8->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_8->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_8->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_8->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_8->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_8->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_8->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_8->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_8->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_8->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_8->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_8->text().toUShort());
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

// ======================== Axis 9 ========================
void Widget::on_btn_SetParam_Axi_9_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi9",ui->lineEdit_Standby_Axi_9->text());

    ui->lineEdit_WorkPostion1_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi9",ui->lineEdit_WorkPostion1_Axi_9->text());

    ui->lineEdit_WorkPostion2_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi9",ui->lineEdit_WorkPostion2_Axi_9->text());

    ui->lineEdit_WorkPostion3_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi9",ui->lineEdit_WorkPostion3_Axi_9->text());

    ui->lineEdit_WorkPostion4_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi9",ui->lineEdit_WorkPostion4_Axi_9->text());

    ui->lineEdit_WorkPostion5_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi9",ui->lineEdit_WorkPostion5_Axi_9->text());

    ui->lineEdit_ManuVel_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi9",ui->lineEdit_ManuVel_Axi_9->text());

    ui->lineEdit_AutoVel_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi9",ui->lineEdit_AutoVel_Axi_9->text());

    ui->lineEdit_Inch_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi9",ui->lineEdit_Inch_Axi_9->text());

    ui->lineEdit_MoveRel_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi9",ui->lineEdit_MoveRel_Axi_9->text());

    ui->lineEdit_MoveAbs_Axi_9->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi9",ui->lineEdit_MoveAbs_Axi_9->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 240;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_9->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_9->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_9->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_9->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_9->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_9->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_9->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_9->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_9->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_9->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_9->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_9->text().toUShort());
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

// ======================== Axis 10 ========================
void Widget::on_btn_SetParam_Axi_10_clicked()
{
    QSettings s("MyCompany", "MyApp");
    ui->lineEdit_Standby_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos_axi10",ui->lineEdit_Standby_Axi_10->text());

    ui->lineEdit_WorkPostion1_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/workPos1_axi10",ui->lineEdit_WorkPostion1_Axi_10->text());

    ui->lineEdit_WorkPostion2_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos2_axi10",ui->lineEdit_WorkPostion2_Axi_10->text());

    ui->lineEdit_WorkPostion3_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos3_axi10",ui->lineEdit_WorkPostion3_Axi_10->text());

    ui->lineEdit_WorkPostion4_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos4_axi10",ui->lineEdit_WorkPostion4_Axi_10->text());

    ui->lineEdit_WorkPostion5_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/waitPos5_axi10",ui->lineEdit_WorkPostion5_Axi_10->text());

    ui->lineEdit_ManuVel_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/ManuVel_axi10",ui->lineEdit_ManuVel_Axi_10->text());

    ui->lineEdit_AutoVel_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/AutoVel_axi10",ui->lineEdit_AutoVel_Axi_10->text());

    ui->lineEdit_Inch_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/Inch_axi10",ui->lineEdit_Inch_Axi_10->text());

    ui->lineEdit_MoveRel_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveRel_axi10",ui->lineEdit_MoveRel_Axi_10->text());

    ui->lineEdit_MoveAbs_Axi_10->setValidator(new QIntValidator(0, 50000, this));
    s.setValue("Params/MoveAb_axi10",ui->lineEdit_MoveAbs_Axi_10->text());

    s.sync();


    const int serverId  = 1;
    const int addr      = 270;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(0, 1);
    unitOn.setValue(1, ui->lineEdit_AutoVel_Axi_10->text().toUShort());
    unitOn.setValue(2, ui->lineEdit_ManuVel_Axi_10->text().toUShort());
    unitOn.setValue(3, ui->lineEdit_Inch_Axi_10->text().toUShort());
    unitOn.setValue(4, ui->lineEdit_Standby_Axi_10->text().toUShort());
    unitOn.setValue(5, ui->lineEdit_WorkPostion1_Axi_10->text().toUShort());
    unitOn.setValue(6, ui->lineEdit_WorkPostion2_Axi_10->text().toUShort());
    unitOn.setValue(7, ui->lineEdit_WorkPostion3_Axi_10->text().toUShort());
    unitOn.setValue(8, ui->lineEdit_WorkPostion4_Axi_10->text().toUShort());
    unitOn.setValue(9, ui->lineEdit_WorkPostion5_Axi_10->text().toUShort());
    unitOn.setValue(10, ui->lineEdit_MoveAbs_Axi_10->text().toUShort());
    unitOn.setValue(11, ui->lineEdit_MoveRel_Axi_10->text().toUShort());
    unitOn.setValue(12, ui->lineEdit_PosId_Axi_10->text().toUShort());
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetParam(1) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetParam(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetParam reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}





void Widget::on_btn_MoveVel_Start_Axi_6_clicked()
{
    const int serverId  = 1;
    const int addr      = 176;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_MoveVel_Close_Axi_6_clicked()
{
    const int serverId  = 2;
    const int addr      = 176;     // 相对定位标识-轴6
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_MoveVel_Start_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 206;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_MoveVel_Close_Axi_7_clicked()
{
    const int serverId  = 1;
    const int addr      = 206;     // 相对定位标识-轴6
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_MoveVel_Start_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 236;     // 相对定位标识-轴6
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_MoveVel_Close_Axi_8_clicked()
{
    const int serverId  = 1;
    const int addr      = 236;     // 相对定位标识-轴6
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send MoveRel(1) request (Axi_6).";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "MoveRel(1) failed (Axi_6):" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send MoveRel(0) request (Axi_6).";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}





void Widget::on_check_OpenCylinder_1_clicked(bool checked) //气缸
{


    quint16 j = checked? 1:2;
    const int serverId  = 1;
    const int addr      = 300;
    const quint16 onVal = j;
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_9).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);



}




void Widget::on_check_OpenCylinder_2_clicked(bool checked) //气缸
{
    quint16 j = checked? 1:2;
    const int serverId  = 1;
    const int addr      = 301;
    const quint16 onVal = j;
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_9).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}


void Widget::on_btn_Putter_Open_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 302;     // 相对定位标识-轴2
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Open_1(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Open_1(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Open_1(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}




void Widget::on_btn_Putter_Close_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 302;     // 相对定位标识-轴2
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Close(2) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Close(2) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Close(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_Putter_Stop_1_clicked()
{
    const int serverId  = 1;
    const int addr      = 302;     // 相对定位标识-轴2
    const quint16 onVal = 3;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Stop_1(3) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Stop_1(3) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Stop_1(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}
// ===================== Putter 2  (addr = 303) =====================
void Widget::on_btn_Putter_Open_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 303;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Open_2(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Open_2(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Open_2(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Putter_Close_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 303;
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Close_2(2) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Close_2(2) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Close_2(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Putter_Stop_2_clicked()
{
    const int serverId  = 1;
    const int addr      = 303;
    const quint16 onVal = 3;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Stop_2(3) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Stop_2(3) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Stop_2(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


// ===================== Putter 3  (addr = 304) =====================
void Widget::on_btn_Putter_Open_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 304;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Open_3(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Open_3(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Open_3(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Putter_Close_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 304;
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Close_3(2) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Close_3(2) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Close_3(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Putter_Stop_3_clicked()
{
    const int serverId  = 1;
    const int addr      = 304;
    const quint16 onVal = 3;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Stop_3(3) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Stop_3(3) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Stop_3(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


// ===================== Putter 4  (addr = 305) =====================
void Widget::on_btn_Putter_Open_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 305;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Open_4(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Open_4(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Open_4(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Putter_Close_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 305;
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Close_4(2) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Close_4(2) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Close_4(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Putter_Stop_4_clicked()
{
    const int serverId  = 1;
    const int addr      = 305;
    const quint16 onVal = 3;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Stop_4(3) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Stop_4(3) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Stop_4(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


// ===================== Putter 5  (addr = 306) =====================
void Widget::on_btn_Putter_Open_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 306;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Open_5(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Open_5(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Open_5(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Putter_Close_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 306;
    const quint16 onVal = 2;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Close_5(2) request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Close_5(2) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Close_5(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}

void Widget::on_btn_Putter_Stop_5_clicked()
{
    const int serverId  = 1;
    const int addr      = 306;
    const quint16 onVal = 3;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send Putter_Stop_5(3) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "Putter_Stop_5(3) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send Putter_Stop_5(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}







void Widget::on_btn_GetBox_clicked()
{
    const int serverId  = 1;
    const int addr      = 340;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send getBox(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "getBox(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send getBox(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_check_GivePower_clicked(bool checked)
{
    quint16 j = checked? 1:0;
    const int serverId  = 1;
    const int addr      = 307;
    const quint16 onVal = j;
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen request (Axi_9).";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);
}


void Widget::on_btn_GetCap_clicked()
{

    const int serverId  = 1;
    const int addr      = 341;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send getBox(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "getBox(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send getBox(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_CapOn_clicked()
{

    const int serverId  = 1;
    const int addr      = 342;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send getBox(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "getBox(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send getBox(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_PutGe_clicked()
{
    const int serverId  = 1;
    const int addr      = 343;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send getBox(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "getBox(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send getBox(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_Store_clicked()
{
    const int serverId  = 1;
    const int addr      = 344;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send getBox(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "getBox(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send getBox(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_CutPaper_clicked()
{
    const int serverId  = 1;
    const int addr      = 345;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send getBox(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "getBox(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send getBox(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_Collect_clicked()
{

    const int serverId  = 1;
    const int addr      = 346;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send getBox(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "getBox(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send getBox(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_All_clicked()
{
    const int serverId  = 1;
    const int addr      = 347;
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;

    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send getBox(1) request .";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "getBox(1) failed :" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to send getBox(0) request .";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_SetSum_Cap_clicked()
{

    const int serverId  = 1;
    const int addr      = 350;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(1, ui->lineEdit_Set_Cap_Sum->text().toUShort());

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetSum_Cap request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetSum_Cap(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetSum_Cap reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_SetSum_Box_clicked()
{
    const int serverId  = 1;
    const int addr      = 352;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(1, ui->lineEdit_Set_Box_Sum->text().toUShort());

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetSum_Cap request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetSum_Cap(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetSum_Cap reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_btn_SetSum_Store_clicked()
{
    const int serverId  = 1;
    const int addr      = 354;     // 触发寄存器
    const quint16 onVal = 1;
    const quint16 offVal= 0;
    const int holdMs    = 100;    // 脉冲保持时间（建议 50~200ms，先用100）

    // 1) 写 addr = 1
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 13);
    unitOn.setValue(0, onVal);
    unitOn.setValue(1, ui->lineEdit_Set_Store_Sum->text().toUShort());

    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SetSum_Cap request.";
        return;
    }

    connect(replyOn, &QModbusReply::finished, this, [=]() {
        if (replyOn->error() != QModbusDevice::NoError) {
            qDebug() << "SetSum_Cap(1) failed:" << replyOn->errorString();
            replyOn->deleteLater();
            return;
        }
        replyOn->deleteLater();

        // 2) 延时 holdMs 后写 addr = 0
        QTimer::singleShot(holdMs, this, [=]() {
            QModbusDataUnit unitOff(QModbusDataUnit::HoldingRegisters, addr, 1);
            unitOff.setValue(0, offVal);

            QModbusReply *replyOff = modbusClient->sendWriteRequest(unitOff, serverId);
            if (!replyOff) {
                qDebug() << "Failed to SetSum_Cap reset(0) request.";
                return;
            }
            connect(replyOff, &QModbusReply::finished, replyOff, &QObject::deleteLater);
        });
    });
}


void Widget::on_check_GetPaper_clicked(bool checked)
{

    quint16 j = checked? 1:2;
    const int serverId  = 1;
    const int addr      = 308;
    const quint16 onVal = j;
    QModbusDataUnit unitOn(QModbusDataUnit::HoldingRegisters, addr, 1);
    unitOn.setValue(0, onVal);
    QModbusReply *replyOn = modbusClient->sendWriteRequest(unitOn, serverId);
    if (!replyOn) {
        qDebug() << "Failed to send SoftOpen on_check_GetPaper_clicked ().";
        return;
    }
    connect(replyOn, &QModbusReply::finished, replyOn, &QObject::deleteLater);

}

