#!/usr/bin/env python3
"""
Smart Lamp Controller - Python Script

Usage: python lamp_control.py [IP_ADDRESS] [COMMAND] [TIMER_MINUTES]
Commands: on, off, status, timer
Examples:
    python lamp_control.py 192.168.1.100 on
    python lamp_control.py 192.168.1.100 timer 30
    python lamp_control.py  # Interactive menu
"""

import sys
import requests
import json
import time
from typing import Optional

# Colors for terminal output
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    BLUE = '\033[0;34m'
    YELLOW = '\033[1;33m'
    CYAN = '\033[0;36m'
    RESET = '\033[0m'

class LampController:
    def __init__(self, ip_address: str):
        self.ip_address = ip_address
        self.base_url = f"http://{ip_address}"
        self.timeout = 5
    
    def _make_request(self, endpoint: str, params: dict = None) -> tuple[bool, Optional[dict]]:
        """Make HTTP request to lamp controller"""
        try:
            url = f"{self.base_url}/{endpoint}"
            response = requests.get(url, params=params, timeout=self.timeout)
            response.raise_for_status()
            
            # Try to parse JSON if response contains it
            try:
                return True, response.json()
            except json.JSONDecodeError:
                return True, None
                
        except requests.exceptions.RequestException as e:
            print(f"{Colors.RED}Error: Could not connect to lamp at {self.ip_address}{Colors.RESET}")
            print(f"Make sure the IP address is correct and lamp is online.")
            print(f"Details: {e}")
            return False, None
    
    def turn_on(self) -> bool:
        """Turn lamp on"""
        print(f"{Colors.YELLOW}Turning lamp ON...{Colors.RESET}")
        success, _ = self._make_request("on")
        if success:
            print(f"{Colors.GREEN}Lamp turned ON successfully!{Colors.RESET}")
        return success
    
    def turn_off(self) -> bool:
        """Turn lamp off"""
        print(f"{Colors.YELLOW}Turning lamp OFF...{Colors.RESET}")
        success, _ = self._make_request("off")
        if success:
            print(f"{Colors.GREEN}Lamp turned OFF successfully!{Colors.RESET}")
        return success
    
    def get_status(self) -> bool:
        """Get lamp status"""
        print(f"{Colors.YELLOW}Getting lamp status...{Colors.RESET}")
        success, data = self._make_request("status")
        
        if success and data:
            print(f"\n{Colors.BLUE}Raw status: {json.dumps(data, indent=2)}{Colors.RESET}\n")
            
            # Display parsed status
            lamp_on = data.get('on', False)
            timer_active = data.get('timeoutActive', False)
            remaining = data.get('remainingSeconds', 0)
            
            status_color = Colors.GREEN if lamp_on else Colors.RED
            status_text = "ON" if lamp_on else "OFF"
            print(f"Lamp is currently {status_color}{status_text}{Colors.RESET}")
            
            if timer_active and remaining > 0:
                minutes = remaining // 60
                seconds = remaining % 60
                print(f"{Colors.YELLOW}Timer is active - Time remaining: {minutes}:{seconds:02d}{Colors.RESET}")
            else:
                print("No timer active")
        
        return success
    
    def set_timer(self, minutes: int) -> bool:
        """Set timer for specified minutes"""
        # Validate minutes
        minutes = max(0, min(720, minutes))
        
        if minutes == 0:
            print(f"{Colors.YELLOW}Cancelling timer...{Colors.RESET}")
        else:
            print(f"{Colors.YELLOW}Setting timer for {minutes} minutes...{Colors.RESET}")
        
        success, _ = self._make_request("timeout", {"minutes": minutes})
        
        if success:
            if minutes == 0:
                print(f"{Colors.GREEN}Timer cancelled successfully!{Colors.RESET}")
            else:
                print(f"{Colors.GREEN}Timer set for {minutes} minutes. Lamp will turn ON and auto-off!{Colors.RESET}")
        
        return success

def clear_screen():
    """Clear terminal screen"""
    import os
    os.system('cls' if os.name == 'nt' else 'clear')

def show_menu(controller: LampController = None):
    """Show interactive menu"""
    lamp_ip = controller.ip_address if controller else None
    
    while True:
        clear_screen()
        print(f"{Colors.BLUE}")
        print("=" * 44)
        print("        Smart Lamp Controller")
        print("=" * 44)
        print(f"{Colors.RESET}")
        
        # Get IP address if not provided
        if not lamp_ip:
            lamp_ip = input("Enter lamp IP address: ").strip()
            if not lamp_ip:
                print(f"{Colors.RED}IP address cannot be empty!{Colors.RESET}")
                input("Press Enter to continue...")
                continue
            controller = LampController(lamp_ip)
        
        print(f"Current lamp IP: {Colors.CYAN}{lamp_ip}{Colors.RESET}")
        print()
        print("1. Turn lamp ON")
        print("2. Turn lamp OFF")
        print("3. Check status")
        print("4. Set timer")
        print("5. Quick timer - 5 minutes")
        print("6. Quick timer - 30 minutes")
        print("7. Quick timer - 1 hour")
        print("8. Cancel timer")
        print("9. Change IP address")
        print("0. Exit")
        print()
        
        choice = input("Select option (0-9): ").strip()
        
        try:
            if choice == "1":
                controller.turn_on()
            elif choice == "2":
                controller.turn_off()
            elif choice == "3":
                controller.get_status()
            elif choice == "4":
                try:
                    minutes = int(input("Enter timer minutes (1-720, 0 to cancel): "))
                except ValueError:
                    minutes = 0
                controller.set_timer(minutes)
            elif choice == "5":
                controller.set_timer(5)
            elif choice == "6":
                controller.set_timer(30)
            elif choice == "7":
                controller.set_timer(60)
            elif choice == "8":
                controller.set_timer(0)
            elif choice == "9":
                lamp_ip = None
                controller = None
                continue
            elif choice == "0":
                print("Goodbye!")
                break
            else:
                print(f"{Colors.RED}Invalid choice!{Colors.RESET}")
        
        except KeyboardInterrupt:
            print(f"\n{Colors.YELLOW}Operation cancelled.{Colors.RESET}")
        except Exception as e:
            print(f"{Colors.RED}Error: {e}{Colors.RESET}")
        
        if choice != "9":
            print()
            input("Press Enter to continue...")

def main():
    """Main function"""
    # Parse command line arguments
    if len(sys.argv) == 1:
        # No arguments - show menu
        show_menu()
        return
    
    if len(sys.argv) < 3:
        print(f"{Colors.RED}Usage: {sys.argv[0]} [IP_ADDRESS] [COMMAND] [TIMER_MINUTES]{Colors.RESET}")
        print("Commands: on, off, status, timer")
        print("Examples:")
        print(f"  {sys.argv[0]} 192.168.1.100 on")
        print(f"  {sys.argv[0]} 192.168.1.100 timer 30")
        print(f"  {sys.argv[0]}  # Interactive menu")
        sys.exit(1)
    
    ip_address = sys.argv[1]
    command = sys.argv[2].lower()
    
    controller = LampController(ip_address)
    
    if command == "on":
        success = controller.turn_on()
    elif command == "off":
        success = controller.turn_off()
    elif command == "status":
        success = controller.get_status()
    elif command == "timer":
        if len(sys.argv) < 4:
            print(f"{Colors.RED}Error: Timer command requires minutes parameter{Colors.RESET}")
            print(f"Usage: {sys.argv[0]} {ip_address} timer [minutes]")
            sys.exit(1)
        
        try:
            minutes = int(sys.argv[3])
        except ValueError:
            print(f"{Colors.RED}Error: Timer minutes must be a number{Colors.RESET}")
            sys.exit(1)
        
        success = controller.set_timer(minutes)
    else:
        print(f"{Colors.RED}Invalid command: {command}{Colors.RESET}")
        print("Valid commands: on, off, status, timer")
        sys.exit(1)
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}Script interrupted by user.{Colors.RESET}")
        sys.exit(0)
    except Exception as e:
        print(f"{Colors.RED}Unexpected error: {e}{Colors.RESET}")
        sys.exit(1)
