/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package gui;

import com.fazecast.jSerialComm.*;
import javax.swing.*;
import java.awt.*;
import javax.swing.border.*;
import org.jfree.chart.*;
import org.jfree.data.xy.*;
import java.util.Scanner;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

/**
 *
 * @author villa
 */
public class GUI {

    static SerialPort serialPort;
    static int x = 0;  // Keep track number of readings detected by serial port
    static int conversions = 0; // Keep track of temp conversions

    Color color = new Color(214, 217, 223); // Cool gray

    Dimension size = new Dimension(100, 25); // Dimension of buttons
    Dimension size2 = new Dimension(50, 25); // Dimension of text fields

    JFrame frame = new JFrame("Backup Power Supply Monitor");

    JPanel panel = new JPanel();
    JPanel panel2 = new JPanel();
    JPanel panel3 = new JPanel();

    JButton button1 = new JButton("Connect");
    JButton button2 = new JButton("Disconnect");

    JLabel label = new JLabel("Port");
    JLabel label2 = new JLabel("Baud Rate");
    JLabel label3 = new JLabel("Internal Temperature");
    JLabel label4 = new JLabel("External Temperature");
    JLabel label5 = new JLabel("Voltage");

    JTextField textField1 = new JTextField();
    JTextField textField3 = new JTextField();
    JTextField textField2 = new JTextField();

    JComboBox comboBox1 = new JComboBox();
    JComboBox comboBox2 = new JComboBox(new String[]{"Select", "2400", "4800",
        "9600", "14400", "19200", "38400", "57600", "115200", "230400",
        "460800", "921600"});
    JComboBox comboBox3 = new JComboBox(new String[]{"°C", "°F"});
    JComboBox comboBox4 = new JComboBox(new String[]{"°C", "°F"});
    JComboBox comboBox5 = new JComboBox(new String[]{"V", "mV"});

    // Captures internal and external temp and coltage reasdings
    XYSeries series1 = new XYSeries("Internal Temperature");
    XYSeries series2 = new XYSeries("External Temperature");
    XYSeries series3 = new XYSeries("Voltage Readings");
    
    // Add temp readings to first dataset
    XYSeriesCollection dataset1 = new XYSeriesCollection(series1);
    JFreeChart chart1 = ChartFactory.createXYAreaChart("Temperature",
            "Readings", "Temperature (°C)", dataset1);
    
    // Add voltage readings to second dataset
    XYSeriesCollection dataset2 = new XYSeriesCollection(series3);
    JFreeChart chart2 = ChartFactory.createXYAreaChart("Voltage",
                "Readingsb", "Volts (V)", dataset2);

    // Set to store internal and external temp, and voltage raedings
    String[] values = new String[3]; 
    // Set default conversions
    Boolean convertTempInter = false, convertTempExter = false, convertV = false;

    public GUI() {
    }

    ;

    public void createGUI() {
        // Get port names, add them to combobox1, and initialize desired port
        // for serial communication
        SerialPort[] portNames = SerialPort.getCommPorts();
        for (int i = 0; i < portNames.length; i++) {
            comboBox1.addItem(portNames[i].getSystemPortName());
        }
        serialPort = SerialPort.getCommPort(comboBox1.getSelectedItem().toString());
        serialPort.setComPortTimeouts(SerialPort.TIMEOUT_SCANNER, 0, 0);

        // Frame properties
        frame.setSize(1375, 750);
        frame.setLocationRelativeTo(null);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLayout(new BorderLayout());

        // Buttons and text fields properties
        button1.setToolTipText("Connect to available serial port");
        button2.setToolTipText("Disconnect serial port");
        button1.setPreferredSize(size);
        button2.setPreferredSize(size);
        textField1.setPreferredSize(size2);
        textField2.setPreferredSize(size2);
        textField3.setPreferredSize(size2);

        // Add functionality to buttons and text fields
        GUIListener myListener = new GUIListener();
        button1.addActionListener(myListener);
        button2.addActionListener(myListener);
        comboBox2.addActionListener(myListener);
        comboBox3.addActionListener(myListener);
        comboBox4.addActionListener(myListener);
        comboBox5.addActionListener(myListener);

        // Add Components and set properties of panels
        panel.setBorder(new TitledBorder("Port Connection"));
        panel.setBackground(color);
        panel.add(label);
        panel.add(comboBox1);
        panel.add(label2);
        panel.add(comboBox2);
        panel.add(button1, BorderLayout.PAGE_END);
        panel.add(button2);

        panel2.setBorder(new TitledBorder("Sensor Monitor"));
        panel2.setBackground(color);
        panel2.add(label3,BorderLayout.PAGE_START);
        panel2.add(textField1,BorderLayout.PAGE_START);
        panel2.add(comboBox3,BorderLayout.PAGE_START);
        panel2.add(label4,BorderLayout.PAGE_START);
        panel2.add(textField2,BorderLayout.PAGE_START);
        panel2.add(comboBox4,BorderLayout.PAGE_START);
        panel2.add(label5,BorderLayout.PAGE_START);
        panel2.add(textField3,BorderLayout.PAGE_START);
        panel2.add(comboBox5,BorderLayout.PAGE_START);

        // Chart properties
        dataset1.addSeries(series2); // Add external temp readings series
    
        chart1.getPlot().setBackgroundPaint(Color.BLACK);
        chart1.setBackgroundPaint(color);
        chart1.setBorderPaint(new Color(153, 214, 255));
        chart1.setBorderVisible(true);
        chart2.getPlot().setBackgroundPaint(Color.BLACK);
        chart2.setBackgroundPaint(color);
        chart2.setBorderPaint(new Color(153, 214, 255));
        chart2.setBorderVisible(true);

        // Add panels to frame
        frame.add(panel, BorderLayout.PAGE_START);
        frame.add(panel2, BorderLayout.PAGE_END);
        frame.add(new ChartPanel(chart1), BorderLayout.WEST);
        frame.add(new ChartPanel(chart2), BorderLayout.EAST);
        frame.setVisible(true);
    }

    // GUIListener class adds functionality to buttons and drop down lists (combobox)
    public class GUIListener implements ActionListener {

        @Override
        public void actionPerformed(ActionEvent e) {
            if (e.getSource() == button1) {
                if (serialPort.openPort()) {
                    System.out.println("Port Connection Successuful!");
                } else {
                    System.out.println("Port Connecttion Failed!");
                    comboBox1.setEnabled(false);
                }
                Thread thread = new Thread() {
                    @Override
                    public void run() {
                        Scanner scanner = new Scanner(serialPort.getInputStream());
                        while (scanner.hasNextLine()) {
                            try {
                                values = scanner.nextLine().split(",");
                                values[2] = Double.toString(Double.parseDouble(values[2])/1000*4);
                                System.out.println(values[0] + " " + values[1] + " " + values[2]);
                                series1.add(x,  Double.parseDouble(values[0]));
                                series2.add(x,  Double.parseDouble(values[1]));
                                series3.add(x++, Double.parseDouble(values[2]));
                                frame.repaint();
                            } catch (Exception e) {
                            }
                            convertTemp(convertTempInter, values[0]);
                            convertTemp(convertTempExter, values[1]);
                            convertVolts(convertV);
                            textField1.setText(values[0]);
                            textField2.setText(values[1]);
                            textField3.setText(values[2]);
                        }
                        scanner.close();
                    }
                };
                thread.start();
            } else if (e.getSource() == button2) {
                serialPort.closePort();
                System.out.println("Port Disconnected!");
                textField1.setText("");
                textField2.setText("");
                textField3.setText("");
                series1.clear();
                series2.clear();
                series3.clear();
                x = 0;
            } else if (e.getSource() == comboBox2) {
                String baudRate = comboBox2.getSelectedItem().toString();
                serialPort.setBaudRate(Integer.parseInt(baudRate));
            } else if (e.getSource() == comboBox3) {
                convertTempInter = comboBox3.getSelectedItem().equals("°F");
            } else if (e.getSource() == comboBox4) {
                convertTempExter = comboBox4.getSelectedItem().equals("°F");
            } else if (e.getSource() == comboBox5) {
                convertV = comboBox5.getSelectedItem().equals("mV");
            }
        }

        // Convert internal and external temps to °F
        public void convertTemp(Boolean convert, String tempReading) {
            if (convert) {
                if (conversions == 0) {
                    // Convert external temp to doudble in °F and store in values[0]
                    double fahrenheit = Double.parseDouble(tempReading) * 9 / 5 + 32;
                    values[0] = Double.toString(fahrenheit);
                    textField3.setText(values[0]);
                    // Increment number of temp conversions so when the function
                    // is called again the external temp is converted
                    conversions++;
                } else {
                     // Convert internal temp to doudble in °F and store in values[1]
                    double fahrenheit = Double.parseDouble(tempReading) * 9 / 5 + 32;
                    values[1] = Double.toString(fahrenheit);
                    textField3.setText(values[1]);
                    // Decrement number of temp conversions so when the function
                    // is called again the internal temp is converted
                    conversions--; 
                } 
            }
        }

        // Convert volts to mV
        public void convertVolts(Boolean convert) {
            if (convert) {
                //Convert volts to mV and store in values[2]
                double volts = Double.parseDouble(values[2])/1000;
                values[2] = Double.toString(volts);
                textField1.setText(values[2]);
            }
        }

    }
}
