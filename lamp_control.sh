#!/bin/bash

# Smart Lamp Controller - Shell Script
# Usage: ./lamp_control.sh [IP_ADDRESS] [COMMAND] [TIMER_MINUTES]
# Commands: on, off, status, timer
# Example: ./lamp_control.sh 192.168.1.100 on
# Example: ./lamp_control.sh 192.168.1.100 timer 30

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if curl is available
if ! command -v curl &> /dev/null; then
    echo -e "${RED}Error: curl is required but not found. Please install curl.${NC}"
    exit 1
fi

# Function to display menu
show_menu() {
    clear
    echo -e "${BLUE}"
    echo "============================================"
    echo "        Smart Lamp Controller"
    echo "============================================"
    echo -e "${NC}"
    
    if [ -z "$LAMP_IP" ]; then
        read -p "Enter lamp IP address: " LAMP_IP
        if [ -z "$LAMP_IP" ]; then
            echo -e "${RED}IP address cannot be empty!${NC}"
            read -p "Press Enter to continue..."
            show_menu
            return
        fi
    fi
    
    echo "Current lamp IP: $LAMP_IP"
    echo
    echo "1. Turn lamp ON"
    echo "2. Turn lamp OFF"
    echo "3. Check status"
    echo "4. Set timer"
    echo "5. Quick timer - 5 minutes"
    echo "6. Quick timer - 30 minutes"
    echo "7. Quick timer - 1 hour"
    echo "8. Cancel timer"
    echo "9. Change IP address"
    echo "0. Exit"
    echo
    read -p "Select option (0-9): " choice
    
    case $choice in
        1) turn_on ;;
        2) turn_off ;;
        3) get_status ;;
        4) set_timer_menu ;;
        5) TIMER_MINUTES=5; set_timer ;;
        6) TIMER_MINUTES=30; set_timer ;;
        7) TIMER_MINUTES=60; set_timer ;;
        8) TIMER_MINUTES=0; set_timer ;;
        9) LAMP_IP=""; show_menu ;;
        0) exit 0 ;;
        *) echo -e "${RED}Invalid choice!${NC}"; read -p "Press Enter to continue..."; show_menu ;;
    esac
}

# Function to set timer with user input
set_timer_menu() {
    echo
    read -p "Enter timer minutes (1-720, 0 to cancel): " TIMER_MINUTES
    if [ -z "$TIMER_MINUTES" ]; then
        TIMER_MINUTES=0
    fi
    set_timer
}

# Function to turn lamp on
turn_on() {
    echo -e "${YELLOW}Turning lamp ON...${NC}"
    if curl -s -f "http://$LAMP_IP/on" > /dev/null; then
        echo -e "${GREEN}Lamp turned ON successfully!${NC}"
    else
        echo -e "${RED}Error: Could not connect to lamp at $LAMP_IP${NC}"
        echo "Make sure the IP address is correct and lamp is online."
    fi
    continue_menu
}

# Function to turn lamp off
turn_off() {
    echo -e "${YELLOW}Turning lamp OFF...${NC}"
    if curl -s -f "http://$LAMP_IP/off" > /dev/null; then
        echo -e "${GREEN}Lamp turned OFF successfully!${NC}"
    else
        echo -e "${RED}Error: Could not connect to lamp at $LAMP_IP${NC}"
        echo "Make sure the IP address is correct and lamp is online."
    fi
    continue_menu
}

# Function to get status
get_status() {
    echo -e "${YELLOW}Getting lamp status...${NC}"
    response=$(curl -s -f "http://$LAMP_IP/status" 2>/dev/null)
    
    if [ $? -ne 0 ]; then
        echo -e "${RED}Error: Could not connect to lamp at $LAMP_IP${NC}"
        echo "Make sure the IP address is correct and lamp is online."
        continue_menu
        return
    fi
    
    echo
    echo -e "${BLUE}Raw status: $response${NC}"
    echo
    
    # Parse JSON response
    if echo "$response" | grep -q '"on":true'; then
        echo -e "${GREEN}Lamp is currently ON${NC}"
    else
        echo -e "${RED}Lamp is currently OFF${NC}"
    fi
    
    if echo "$response" | grep -q '"timeoutActive":true'; then
        echo -e "${YELLOW}Timer is active${NC}"
        # Extract remaining time if available
        remaining=$(echo "$response" | grep -o '"remainingSeconds":[0-9]*' | cut -d':' -f2)
        if [ ! -z "$remaining" ] && [ "$remaining" -gt 0 ]; then
            minutes=$((remaining / 60))
            seconds=$((remaining % 60))
            echo -e "${YELLOW}Time remaining: ${minutes}:$(printf "%02d" $seconds)${NC}"
        fi
    else
        echo "No timer active"
    fi
    
    continue_menu
}

# Function to set timer
set_timer() {
    # Validate timer minutes
    if ! [[ "$TIMER_MINUTES" =~ ^[0-9]+$ ]] || [ "$TIMER_MINUTES" -lt 0 ]; then
        TIMER_MINUTES=0
    fi
    
    if [ "$TIMER_MINUTES" -gt 720 ]; then
        TIMER_MINUTES=720
    fi
    
    if [ "$TIMER_MINUTES" -eq 0 ]; then
        echo -e "${YELLOW}Cancelling timer...${NC}"
    else
        echo -e "${YELLOW}Setting timer for $TIMER_MINUTES minutes...${NC}"
    fi
    
    if curl -s -f "http://$LAMP_IP/timeout?minutes=$TIMER_MINUTES" > /dev/null; then
        if [ "$TIMER_MINUTES" -eq 0 ]; then
            echo -e "${GREEN}Timer cancelled successfully!${NC}"
        else
            echo -e "${GREEN}Timer set for $TIMER_MINUTES minutes. Lamp will turn ON and auto-off!${NC}"
        fi
    else
        echo -e "${RED}Error: Could not connect to lamp at $LAMP_IP${NC}"
        echo "Make sure the IP address is correct and lamp is online."
    fi
    
    continue_menu
}

# Function to continue or return to menu
continue_menu() {
    if [ -z "$1" ]; then
        echo
        read -p "Press Enter to continue..."
        show_menu
    fi
}

# Main script logic
LAMP_IP="$1"
COMMAND="$2"
TIMER_MINUTES="$3"

# If no arguments, show menu
if [ -z "$LAMP_IP" ] || [ -z "$COMMAND" ]; then
    show_menu
    exit 0
fi

# Execute command based on arguments
case $COMMAND in
    "on")
        turn_on
        exit 0
        ;;
    "off")
        turn_off
        exit 0
        ;;
    "status")
        get_status
        exit 0
        ;;
    "timer")
        if [ -z "$TIMER_MINUTES" ]; then
            echo -e "${RED}Error: Timer command requires minutes parameter${NC}"
            echo "Usage: $0 $LAMP_IP timer [minutes]"
            exit 1
        fi
        set_timer
        exit 0
        ;;
    *)
        echo -e "${RED}Invalid command: $COMMAND${NC}"
        echo "Valid commands: on, off, status, timer"
        echo "Usage: $0 [IP_ADDRESS] [COMMAND] [TIMER_MINUTES]"
        exit 1
        ;;
esac
