#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDebug>//用于在控制台输出调试信息
#include <QMessageBox>
#include <Windows.h>
// #include "TcAdsDef.h"
// #include "TcAdsAPI.h"
#include <QElapsedTimer>
#include <QTimer>
#include <QString>
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>
#include <QModbusTcpClient>
#include <QModbusDataUnit>
#include <QSettings>
#include <QIntValidator>
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
class PlcStruct_Read;
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    bool m_readingAxis1 = false;
    bool m_readingPlc = false;
    void onModbusStateChanged(int);
    void onReadFinished2();
    void onReadFinished3();
    void onWriteFinished();
    void loadParams_Axi_1();
    void readPlcState();






    bool m_readingAxis_1to5  = false;
    bool m_readingAxis_6to10 = false;

    struct AxisConfig {
        int axisNo;        // 1..10
        int startAddr;     // 起始保持寄存器
        int count;         // 读取长度
        std::function<void(const QModbusDataUnit&)> apply; // 解析+更新UI
    };

    QVector<AxisConfig> m_axisCfg;
    QVector<bool> m_axisReading;   // 防重入（按配置索引）
    QTimer* m_pollTimer = nullptr;
    int m_pollIndex = 0;           // 轮询到第几个cfg

private slots:

    void readAxisState_1to5();
    void readAxisState_6to10();

    void on_btn_connect_plc_clicked();  // 连接PLC
    void on_check_OpenCylinder_1_clicked(bool checked); //气缸1
    void on_check_OpenCylinder_2_clicked(bool checked); //气缸2
    void on_btn_Axi_All_Start_clicked();
    void on_btn_Axi_All_GoHome_clicked();
    void on_btn_Axi_ALL_Reset_clicked();
    void on_btn_Axi_All_Stop_clicked();
    void Set_On_Auto();
    void Set_On_Manu();
    void on_btn_Axi_Mode_Switch_clicked();
    void on_ManuButton_clicked(bool checked);
    void on_AutoButton_clicked(bool checked);


    void on_btn_Enable_Open_Axi_1_clicked();
    void on_btn_Enable_Close_Axi_1_clicked();
    void on_btn_Stop_Axi_1_clicked();
    void on_btn_Reset_Axi_1_clicked();

    void on_btn_SetParam_Axi_1_clicked();
    void onWriteParamFinished_Axi_1();
    void on_btn_SetHome_Axi_1_clicked();
    void on_btn_GoHome_Axi_1_clicked();
    void on_btn_Jog_SoftOpen_Axi_1_clicked();

    void on_btn_Jog_SoftClose_Axi_1_clicked();

    void on_btn_Jog_Jog_Axi_1_clicked();

    void on_btn_Jog_Inch_Axi_1_clicked();

    void on_btn_MoveAbs_Axi_1_clicked();

    void on_btn_MoveRel_Axi_1_clicked();



    void on_btn_Jog_Neg_Axi_1_pressed();

    void on_btn_Jog_Pos_Axi_1_pressed();

    void on_btn_Jog_Neg_Axi_1_released();

    void on_btn_Jog_Pos_Axi_1_released();



    void on_btn_SetParam_Axi_2_clicked();
    void on_btn_Enable_Open_Axi_2_clicked();
    void on_btn_Enable_Close_Axi_2_clicked();
    void on_btn_Jog_Neg_Axi_2_pressed();
    void on_btn_Jog_Neg_Axi_2_released();
    void on_btn_Jog_Pos_Axi_2_pressed();
    void on_btn_Jog_Pos_Axi_2_released();
    void on_btn_Stop_Axi_2_clicked();
    void on_btn_Reset_Axi_2_clicked();
    void on_btn_GoHome_Axi_2_clicked();
    void on_btn_MoveAbs_Axi_2_clicked();
    void on_btn_MoveRel_Axi_2_clicked();
    void on_btn_SetHome_Axi_2_clicked();
    void on_btn_Jog_SoftOpen_Axi_2_clicked();
    void on_btn_Jog_SoftClose_Axi_2_clicked();
    void on_btn_Jog_Jog_Axi_2_clicked();
    void on_btn_Jog_Inch_Axi_2_clicked();

    void on_btn_Enable_Open_Axi_3_clicked();
    void on_btn_Enable_Close_Axi_3_clicked();
    void on_btn_Jog_Neg_Axi_3_pressed();
    void on_btn_Jog_Neg_Axi_3_released();
    void on_btn_Jog_Pos_Axi_3_pressed();
    void on_btn_Jog_Pos_Axi_3_released();
    void on_btn_Stop_Axi_3_clicked();
    void on_btn_Reset_Axi_3_clicked();
    void on_btn_GoHome_Axi_3_clicked();
    void on_btn_MoveAbs_Axi_3_clicked();
    void on_btn_MoveRel_Axi_3_clicked();
    void on_btn_SetHome_Axi_3_clicked();
    void on_btn_Jog_SoftOpen_Axi_3_clicked();
    void on_btn_Jog_SoftClose_Axi_3_clicked();
    void on_btn_Jog_Jog_Axi_3_clicked();
    void on_btn_Jog_Inch_Axi_3_clicked();

    void on_btn_Enable_Open_Axi_4_clicked();
    void on_btn_Enable_Close_Axi_4_clicked();
    void on_btn_Jog_Neg_Axi_4_pressed();
    void on_btn_Jog_Neg_Axi_4_released();
    void on_btn_Jog_Pos_Axi_4_pressed();
    void on_btn_Jog_Pos_Axi_4_released();
    void on_btn_Stop_Axi_4_clicked();
    void on_btn_Reset_Axi_4_clicked();
    void on_btn_GoHome_Axi_4_clicked();
    void on_btn_MoveAbs_Axi_4_clicked();
    void on_btn_MoveRel_Axi_4_clicked();
    void on_btn_SetHome_Axi_4_clicked();
    void on_btn_Jog_SoftOpen_Axi_4_clicked();
    void on_btn_Jog_SoftClose_Axi_4_clicked();
    void on_btn_Jog_Jog_Axi_4_clicked();
    void on_btn_Jog_Inch_Axi_4_clicked();

    void on_btn_Enable_Open_Axi_5_clicked();
    void on_btn_Enable_Close_Axi_5_clicked();
    void on_btn_Jog_Neg_Axi_5_pressed();
    void on_btn_Jog_Neg_Axi_5_released();
    void on_btn_Jog_Pos_Axi_5_pressed();
    void on_btn_Jog_Pos_Axi_5_released();
    void on_btn_Stop_Axi_5_clicked();
    void on_btn_Reset_Axi_5_clicked();
    void on_btn_GoHome_Axi_5_clicked();
    void on_btn_MoveAbs_Axi_5_clicked();
    void on_btn_MoveRel_Axi_5_clicked();
    void on_btn_SetHome_Axi_5_clicked();
    void on_btn_Jog_SoftOpen_Axi_5_clicked();
    void on_btn_Jog_SoftClose_Axi_5_clicked();
    void on_btn_Jog_Jog_Axi_5_clicked();
    void on_btn_Jog_Inch_Axi_5_clicked();

    void on_btn_Enable_Open_Axi_6_clicked();
    void on_btn_Enable_Close_Axi_6_clicked();
    void on_btn_Jog_Neg_Axi_6_pressed();
    void on_btn_Jog_Neg_Axi_6_released();
    void on_btn_Jog_Pos_Axi_6_pressed();
    void on_btn_Jog_Pos_Axi_6_released();
    void on_btn_Stop_Axi_6_clicked();
    void on_btn_Reset_Axi_6_clicked();
    void on_btn_GoHome_Axi_6_clicked();
    void on_btn_MoveAbs_Axi_6_clicked();
    void on_btn_MoveRel_Axi_6_clicked();
    void on_btn_SetHome_Axi_6_clicked();
    void on_btn_Jog_SoftOpen_Axi_6_clicked();
    void on_btn_Jog_SoftClose_Axi_6_clicked();
    void on_btn_Jog_Jog_Axi_6_clicked();
    void on_btn_Jog_Inch_Axi_6_clicked();

    void on_btn_Enable_Open_Axi_7_clicked();
    void on_btn_Enable_Close_Axi_7_clicked();
    void on_btn_Jog_Neg_Axi_7_pressed();
    void on_btn_Jog_Neg_Axi_7_released();
    void on_btn_Jog_Pos_Axi_7_pressed();
    void on_btn_Jog_Pos_Axi_7_released();
    void on_btn_Stop_Axi_7_clicked();
    void on_btn_Reset_Axi_7_clicked();
    void on_btn_GoHome_Axi_7_clicked();
    void on_btn_MoveAbs_Axi_7_clicked();
    void on_btn_MoveRel_Axi_7_clicked();
    void on_btn_SetHome_Axi_7_clicked();
    void on_btn_Jog_SoftOpen_Axi_7_clicked();
    void on_btn_Jog_SoftClose_Axi_7_clicked();
    void on_btn_Jog_Jog_Axi_7_clicked();
    void on_btn_Jog_Inch_Axi_7_clicked();

    void on_btn_Enable_Open_Axi_8_clicked();
    void on_btn_Enable_Close_Axi_8_clicked();
    void on_btn_Jog_Neg_Axi_8_pressed();
    void on_btn_Jog_Neg_Axi_8_released();
    void on_btn_Jog_Pos_Axi_8_pressed();
    void on_btn_Jog_Pos_Axi_8_released();
    void on_btn_Stop_Axi_8_clicked();
    void on_btn_Reset_Axi_8_clicked();
    void on_btn_GoHome_Axi_8_clicked();
    void on_btn_MoveAbs_Axi_8_clicked();
    void on_btn_MoveRel_Axi_8_clicked();
    void on_btn_SetHome_Axi_8_clicked();
    void on_btn_Jog_SoftOpen_Axi_8_clicked();
    void on_btn_Jog_SoftClose_Axi_8_clicked();
    void on_btn_Jog_Jog_Axi_8_clicked();
    void on_btn_Jog_Inch_Axi_8_clicked();

    void on_btn_Enable_Open_Axi_9_clicked();
    void on_btn_Enable_Close_Axi_9_clicked();
    void on_btn_Jog_Neg_Axi_9_pressed();
    void on_btn_Jog_Neg_Axi_9_released();
    void on_btn_Jog_Pos_Axi_9_pressed();
    void on_btn_Jog_Pos_Axi_9_released();
    void on_btn_Stop_Axi_9_clicked();
    void on_btn_Reset_Axi_9_clicked();
    void on_btn_GoHome_Axi_9_clicked();
    void on_btn_MoveAbs_Axi_9_clicked();
    void on_btn_MoveRel_Axi_9_clicked();
    void on_btn_SetHome_Axi_9_clicked();
    void on_btn_Jog_SoftOpen_Axi_9_clicked();
    void on_btn_Jog_SoftClose_Axi_9_clicked();
    void on_btn_Jog_Jog_Axi_9_clicked();
    void on_btn_Jog_Inch_Axi_9_clicked();

    void on_btn_Enable_Open_Axi_10_clicked();
    void on_btn_Enable_Close_Axi_10_clicked();
    void on_btn_Jog_Neg_Axi_10_pressed();
    void on_btn_Jog_Neg_Axi_10_released();
    void on_btn_Jog_Pos_Axi_10_pressed();
    void on_btn_Jog_Pos_Axi_10_released();
    void on_btn_Stop_Axi_10_clicked();
    void on_btn_Reset_Axi_10_clicked();
    void on_btn_GoHome_Axi_10_clicked();
    void on_btn_MoveAbs_Axi_10_clicked();
    void on_btn_MoveRel_Axi_10_clicked();
    void on_btn_SetHome_Axi_10_clicked();
    void on_btn_Jog_SoftOpen_Axi_10_clicked();
    void on_btn_Jog_SoftClose_Axi_10_clicked();
    void on_btn_Jog_Jog_Axi_10_clicked();
    void on_btn_Jog_Inch_Axi_10_clicked();

    void on_btn_SetParam_Axi_3_clicked();
    void on_btn_SetParam_Axi_4_clicked();
    void on_btn_SetParam_Axi_5_clicked();
    void on_btn_SetParam_Axi_6_clicked();
    void on_btn_SetParam_Axi_7_clicked();
    void on_btn_SetParam_Axi_8_clicked();
    void on_btn_SetParam_Axi_9_clicked();
    void on_btn_SetParam_Axi_10_clicked();







    void on_btn_MoveVel_Start_Axi_6_clicked();

    void on_btn_MoveVel_Close_Axi_6_clicked();

    void on_btn_MoveVel_Start_Axi_7_clicked();

    void on_btn_MoveVel_Close_Axi_7_clicked();

    void on_btn_MoveVel_Start_Axi_8_clicked();

    void on_btn_MoveVel_Close_Axi_8_clicked();

    void on_btn_Axi_All_Enable_clicked();

    void on_btn_Axi_All_CloseEnable_clicked();

    void on_btn_Putter_Open_1_clicked();

    void on_btn_Putter_Close_1_clicked();

    void on_btn_Putter_Stop_1_clicked();
    // ---- Putter 2 ----
    void on_btn_Putter_Open_2_clicked();
    void on_btn_Putter_Close_2_clicked();
    void on_btn_Putter_Stop_2_clicked();

    // ---- Putter 3 ----
    void on_btn_Putter_Open_3_clicked();
    void on_btn_Putter_Close_3_clicked();
    void on_btn_Putter_Stop_3_clicked();

    // ---- Putter 4 ----
    void on_btn_Putter_Open_4_clicked();
    void on_btn_Putter_Close_4_clicked();
    void on_btn_Putter_Stop_4_clicked();

    // ---- Putter 5 ----
    void on_btn_Putter_Open_5_clicked();
    void on_btn_Putter_Close_5_clicked();
    void on_btn_Putter_Stop_5_clicked();




    void on_btn_GetBox_clicked();

    void on_check_GivePower_clicked(bool checked);

    void on_btn_GetCap_clicked();

    void on_btn_CapOn_clicked();

    void on_btn_PutGe_clicked();

    void on_btn_Store_clicked();

    void on_btn_CutPaper_clicked();

    void on_btn_Collect_clicked();

    void on_btn_All_clicked();

    void on_btn_SetSum_Cap_clicked();

    void on_btn_SetSum_Box_clicked();

    void on_btn_SetSum_Store_clicked();



    void on_check_GetPaper_clicked(bool checked);


private:
    Ui::Widget *ui;
    // Modbus tcp
    QModbusClient *modbusClient = nullptr;
    QTimer timer;
    QTimer writeTimer;
    // 定时器
    QTimer *_pTimerUpdate,*_pTimerUpdate1;
    QTimer *_pTimerUpdate_setting;// 保存数据定时器
};
#endif // WIDGET_H
