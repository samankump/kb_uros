#!/usr/bin/env python3
"""
KidBright 32 micro-ROS Monitor & Controller CLI
Runs on Ubuntu 26.04 WSL (ROS 2 Kilted / Lyrical)

Features:
- Subscribes to /kidbright/board1/button and /kidbright/board2/button
- Displays live, colorized status and events
- Allows sending commands to Board 1 or Board 2 directly from PC keyboard
"""

import sys
import rclpy
from rclpy.node import Node
from std_msgs.msg import Int32

# ANSI Color codes
C_RESET = "\033[0m"
C_BOLD = "\033[1m"
C_RED = "\033[31m"
C_GREEN = "\033[32m"
C_YELLOW = "\033[33m"
C_BLUE = "\033[34m"
C_MAGENTA = "\033[35m"
C_CYAN = "\033[36m"

class KidbrightMonitor(Node):
    def __init__(self):
        super().__init__('kidbright_monitor')

        # Subscriptions
        self.sub_b1 = self.create_subscription(
            Int32,
            '/kidbright/board1/button',
            self.on_board1_button,
            10
        )
        self.sub_b2 = self.create_subscription(
            Int32,
            '/kidbright/board2/button',
            self.on_board2_button,
            10
        )

        # Publishers (Allow host to send commands simulating boards)
        self.pub_b1 = self.create_publisher(Int32, '/kidbright/board1/button', 10)
        self.pub_b2 = self.create_publisher(Int32, '/kidbright/board2/button', 10)

        self.print_header()

    def print_header(self):
        print(f"{C_CYAN}{C_BOLD}===================================================={C_RESET}")
        print(f"{C_CYAN}{C_BOLD}     KidBright 32 micro-ROS Interactive Monitor     {C_RESET}")
        print(f"{C_CYAN}{C_BOLD}===================================================={C_RESET}")
        print(f"Monitoring Topics:")
        print(f"  • {C_GREEN}/kidbright/board1/button{C_RESET} (Publisher: Board 1)")
        print(f"  • {C_BLUE}/kidbright/board2/button{C_RESET} (Publisher: Board 2)")
        print(f"----------------------------------------------------")
        print(f"Interactive Controls (Type number + Enter):")
        print(f"  [1] Trigger Board 2 reaction (Heart ❤️  + Happy Chime)")
        print(f"  [2] Trigger Board 2 reaction (Smile 😊 + Alert Beep)")
        print(f"  [3] Trigger Board 1 reaction (Heart ❤️  + Happy Chime)")
        print(f"  [4] Trigger Board 1 reaction (Smile 😊 + Alert Beep)")
        print(f"  [q] Quit")
        print(f"{C_CYAN}===================================================={C_RESET}\n")

    def on_board1_button(self, msg: Int32):
        val = msg.data
        if val == 1:
            desc = f"{C_GREEN}Switch 1 Pressed!{C_RESET} -> Sent to Board 2: {C_RED}Heart ❤️  + Chime{C_RESET}"
        elif val == 2:
            desc = f"{C_YELLOW}Switch 2 Pressed!{C_RESET} -> Sent to Board 2: {C_YELLOW}Smile 😊 + Alert{C_RESET}"
        else:
            desc = f"Data = {val}"

        print(f"[{C_GREEN}BOARD 1 EVENT{C_RESET}] {desc}")

    def on_board2_button(self, msg: Int32):
        val = msg.data
        if val == 1:
            desc = f"{C_BLUE}Switch 1 Pressed!{C_RESET} -> Sent to Board 1: {C_RED}Heart ❤️  + Chime{C_RESET}"
        elif val == 2:
            desc = f"{C_MAGENTA}Switch 2 Pressed!{C_RESET} -> Sent to Board 1: {C_YELLOW}Smile 😊 + Alert{C_RESET}"
        else:
            desc = f"Data = {val}"

        print(f"[{C_BLUE}BOARD 2 EVENT{C_RESET}] {desc}")

    def send_cmd(self, target_board: int, button_num: int):
        msg = Int32()
        msg.data = button_num
        if target_board == 2:
            # We publish as board1 so board2 receives it
            self.pub_b1.publish(msg)
            print(f"{C_YELLOW}>>> Sent from Host to Board 2: Button {button_num}{C_RESET}")
        else:
            # We publish as board2 so board1 receives it
            self.pub_b2.publish(msg)
            print(f"{C_YELLOW}>>> Sent from Host to Board 1: Button {button_num}{C_RESET}")

def main():
    rclpy.init()
    node = KidbrightMonitor()

    import threading
    import select

    # Background spin thread for ROS 2 callbacks
    spin_thread = threading.Thread(target=rclpy.spin, args=(node,), daemon=True)
    spin_thread.start()

    try:
        while rclpy.ok():
            if select.select([sys.stdin], [], [], 0.1)[0]:
                line = sys.stdin.readline().strip()
                if line == '1':
                    node.send_cmd(target_board=2, button_num=1)
                elif line == '2':
                    node.send_cmd(target_board=2, button_num=2)
                elif line == '3':
                    node.send_cmd(target_board=1, button_num=1)
                elif line == '4':
                    node.send_cmd(target_board=1, button_num=2)
                elif line.lower() == 'q':
                    break
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
