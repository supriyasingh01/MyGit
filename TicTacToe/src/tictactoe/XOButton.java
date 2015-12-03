/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package tictactoe;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.ImageIcon;
import javax.swing.JButton;

/**
 *
 * @author supriyasingh
 */
public class XOButton extends JButton implements ActionListener {

    ImageIcon X, O;
    byte value = 0;// 0= nothing, 1= X, 2 =O

    public XOButton() {
        X = new ImageIcon(this.getClass().getResource("circle.png"));
        O = new ImageIcon(this.getClass().getResource("X_G.png"));
        this.addActionListener(this); // button is triggering action and listening also.
    }

    public void actionPerformed(ActionEvent e) {
        value++;
        value %= 3;

        switch (value) {
            case 0:
                setIcon(null);
                break;
            case 1:
                setIcon(X);
                break;
            case 2:
                setIcon(O);
                break;
        }
    }
}
