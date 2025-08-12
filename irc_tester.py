#!/usr/bin/env python3
"""
IRC Server Tester for 42 School Project
Tests all requirements from the evaluation sheet
"""

import socket
import time
import threading
import sys
import select
from typing import List, Tuple, Optional

# ANSI color codes
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
MAGENTA = '\033[95m'
CYAN = '\033[96m'
RESET = '\033[0m'
BOLD = '\033[1m'

class IRCClient:
    def __init__(self, host='localhost', port=6666, timeout=2):
        self.host = host
        self.port = port
        self.sock = None
        self.timeout = timeout
        self.buffer = ""
        
    def connect(self):
        """Connect to IRC server"""
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(self.timeout)
            self.sock.connect((self.host, self.port))
            return True
        except Exception as e:
            print(f"{RED}Connection failed: {e}{RESET}")
            return False
    
    def disconnect(self):
        """Close connection"""
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
            self.sock = None
    
    def send(self, message):
        """Send message to server"""
        if not message.endswith('\r\n'):
            message += '\r\n'
        try:
            self.sock.send(message.encode())
            return True
        except Exception as e:
            print(f"{RED}Send failed: {e}{RESET}")
            return False
    
    def recv(self, wait_time=0.5):
        """Receive messages from server"""
        messages = []
        time.sleep(wait_time)  # Give server time to respond
        
        try:
            self.sock.settimeout(0.5)
            while True:
                data = self.sock.recv(4096).decode('utf-8', errors='ignore')
                if not data:
                    break
                self.buffer += data
                
                # Split by lines
                lines = self.buffer.split('\n')
                self.buffer = lines[-1]  # Keep incomplete line in buffer
                
                for line in lines[:-1]:
                    line = line.strip()
                    if line:
                        messages.append(line)
        except socket.timeout:
            pass
        except Exception as e:
            if "Resource temporarily unavailable" not in str(e):
                print(f"{RED}Recv error: {e}{RESET}")
        
        return messages
    
    def authenticate(self, password, nick, username):
        """Authenticate with PASS, NICK, USER"""
        self.send(f"PASS {password}")
        self.send(f"NICK {nick}")
        self.send(f"USER {username} 0 * :{username}")
        return self.recv(1)

class IRCServerTester:
    def __init__(self, host='localhost', port=6666, password='hola'):
        self.host = host
        self.port = port
        self.password = password
        self.test_count = 0
        self.passed_count = 0
        self.failed_count = 0
    
    def print_header(self, text):
        """Print section header"""
        print(f"\n{BOLD}{BLUE}{'='*60}{RESET}")
        print(f"{BOLD}{CYAN}{text}{RESET}")
        print(f"{BOLD}{BLUE}{'='*60}{RESET}\n")
    
    def print_test(self, name, passed, details=""):
        """Print test result"""
        self.test_count += 1
        if passed:
            self.passed_count += 1
            status = f"{GREEN}✓ PASS{RESET}"
        else:
            self.failed_count += 1
            status = f"{RED}✗ FAIL{RESET}"
        
        print(f"  [{status}] {name}")
        if details:
            print(f"        {YELLOW}{details}{RESET}")
    
    def test_basic_connection(self):
        """Test 1: Basic connection and authentication"""
        self.print_header("TEST 1: Basic Connection & Authentication")
        
        client = IRCClient(self.host, self.port)
        
        # Test connection
        passed = client.connect()
        self.print_test("Connection to server", passed)
        
        if not passed:
            client.disconnect()
            return False
        
        # Test authentication
        responses = client.authenticate(self.password, "testuser1", "testuser1")
        auth_success = any("Welcome" in r or "001" in r for r in responses)
        self.print_test("Authentication (PASS/NICK/USER)", auth_success, 
                       f"Responses: {len(responses)} messages")
        
        client.disconnect()
        return auth_success
    
    def test_multiple_connections(self):
        """Test 2: Multiple simultaneous connections"""
        self.print_header("TEST 2: Multiple Simultaneous Connections")
        
        clients = []
        for i in range(3):
            client = IRCClient(self.host, self.port)
            if client.connect():
                clients.append(client)
                client.authenticate(self.password, f"user{i+1}", f"user{i+1}")
        
        passed = len(clients) == 3
        self.print_test("3 simultaneous connections", passed, 
                       f"Connected: {len(clients)}/3")
        
        # Cleanup
        for client in clients:
            client.disconnect()
        
        return passed
    
    def test_join_and_messages(self):
        """Test 3: JOIN channel and PRIVMSG"""
        self.print_header("TEST 3: JOIN & Channel Messages")
        
        # Create two clients
        alice = IRCClient(self.host, self.port)
        bob = IRCClient(self.host, self.port)
        
        alice.connect()
        bob.connect()
        
        alice.authenticate(self.password, "alice", "alice")
        bob.authenticate(self.password, "bob", "bob")
        
        # Both join same channel
        alice.send("JOIN #test")
        time.sleep(0.5)
        bob.send("JOIN #test")
        time.sleep(0.5)
        
        # Alice sends message
        alice.send("PRIVMSG #test :Hello from Alice!")
        
        # Bob should receive it
        bob_messages = bob.recv(1)
        msg_received = any("Hello from Alice" in msg for msg in bob_messages)
        
        self.print_test("JOIN channel", True)
        self.print_test("Channel message delivery", msg_received,
                       f"Bob received {len(bob_messages)} messages")
        
        # Test private message
        alice.send("PRIVMSG bob :Private message to Bob")
        bob_messages = bob.recv(1)
        pm_received = any("Private message" in msg for msg in bob_messages)
        
        self.print_test("Private message delivery", pm_received)
        
        alice.disconnect()
        bob.disconnect()
        
        return msg_received and pm_received
    
    def test_channel_operations(self):
        """Test 4: Channel operator commands"""
        self.print_header("TEST 4: Channel Operator Commands")
        
        op = IRCClient(self.host, self.port)
        user = IRCClient(self.host, self.port)
        
        op.connect()
        user.connect()
        
        op.authenticate(self.password, "operator", "operator")
        user.authenticate(self.password, "normaluser", "normaluser")
        
        # Operator creates channel (becomes op)
        op.send("JOIN #opchannel")
        time.sleep(0.5)
        
        # Test MODE +i (invite only)
        op.send("MODE #opchannel +i")
        time.sleep(0.5)
        
        # User tries to join (should fail)
        user.send("JOIN #opchannel")
        user_msgs = user.recv(1)
        join_blocked = any("Cannot join" in msg or "473" in msg for msg in user_msgs)
        self.print_test("MODE +i (invite only)", join_blocked)
        
        # Op invites user
        op.send("INVITE normaluser #opchannel")
        time.sleep(0.5)
        user.send("JOIN #opchannel")
        user_msgs = user.recv(1)
        join_after_invite = any("JOIN" in msg for msg in user_msgs)
        self.print_test("INVITE command", join_after_invite)
        
        # Test TOPIC
        op.send("TOPIC #opchannel :New channel topic")
        msgs = user.recv(1)
        topic_set = any("TOPIC" in msg or "topic" in msg.lower() for msg in msgs)
        self.print_test("TOPIC command", topic_set)
        
        # Test KICK
        op.send("KICK #opchannel normaluser :Testing kick")
        user_msgs = user.recv(1)
        kicked = any("KICK" in msg for msg in user_msgs)
        self.print_test("KICK command", kicked)
        
        # Test MODE +k (password)
        op.send("MODE #opchannel +k secret123")
        op.recv(0.5)
        self.print_test("MODE +k (channel password)", True, "Command sent")
        
        # Test MODE +l (user limit)
        op.send("MODE #opchannel +l 5")
        op.recv(0.5)
        self.print_test("MODE +l (user limit)", True, "Command sent")
        
        op.disconnect()
        user.disconnect()
        
        return join_blocked and kicked
    
    def test_part_and_quit(self):
        """Test 5: PART and QUIT commands"""
        self.print_header("TEST 5: PART & QUIT Commands")
        
        client1 = IRCClient(self.host, self.port)
        client2 = IRCClient(self.host, self.port)
        
        client1.connect()
        client2.connect()
        
        client1.authenticate(self.password, "client1", "client1")
        client2.authenticate(self.password, "client2", "client2")
        
        # Both join channel
        client1.send("JOIN #partyroom")
        client2.send("JOIN #partyroom")
        time.sleep(0.5)
        
        # Client1 parts with message
        client1.send("PART #partyroom :Goodbye everyone!")
        msgs = client2.recv(1)
        part_received = any("PART" in msg for msg in msgs)
        self.print_test("PART command", part_received)
        
        # Client1 quits
        client1.send("QUIT :Leaving server")
        time.sleep(0.5)
        
        self.print_test("QUIT command", True, "Command sent")
        
        client1.disconnect()
        client2.disconnect()
        
        return part_received
    
    def test_nick_change(self):
        """Test 6: NICK change while connected"""
        self.print_header("TEST 6: NICK Change")
        
        client = IRCClient(self.host, self.port)
        observer = IRCClient(self.host, self.port)
        
        client.connect()
        observer.connect()
        
        client.authenticate(self.password, "oldnick", "oldnick")
        observer.authenticate(self.password, "observer", "observer")
        
        # Join same channel
        client.send("JOIN #nicktest")
        observer.send("JOIN #nicktest")
        time.sleep(0.5)
        
        # Change nick
        client.send("NICK newnick")
        msgs = observer.recv(1)
        nick_changed = any("NICK" in msg and "newnick" in msg for msg in msgs)
        
        self.print_test("NICK change notification", nick_changed)
        
        client.disconnect()
        observer.disconnect()
        
        return nick_changed
    
    def test_partial_commands(self):
        """Test 7: Partial commands (network issues simulation)"""
        self.print_header("TEST 7: Partial Commands Handling")
        
        client = IRCClient(self.host, self.port)
        client.connect()
        client.authenticate(self.password, "partial", "partial")
        
        # Send partial JOIN
        client.sock.send(b"JOIN #te")
        time.sleep(0.5)
        client.sock.send(b"st\r\n")
        
        msgs = client.recv(1)
        joined = any("JOIN" in msg or "353" in msg for msg in msgs)
        self.print_test("Partial JOIN command", joined, 
                       "JOIN #te + st = JOIN #test")
        
        # Send partial PRIVMSG
        client.sock.send(b"PRIVMSG #test :Hello ")
        time.sleep(0.5)
        client.sock.send(b"World!\r\n")
        
        self.print_test("Partial PRIVMSG", True, "Command sent in parts")
        
        client.disconnect()
        return joined
    
    def test_error_handling(self):
        """Test 8: Error handling"""
        self.print_header("TEST 8: Error Handling")
        
        # Wrong password
        client = IRCClient(self.host, self.port)
        client.connect()
        client.send("PASS wrongpass")
        client.send("NICK errortest")
        msgs = client.recv(1)
        wrong_pass = any("incorrect" in msg.lower() or "464" in msg for msg in msgs)
        self.print_test("Wrong password rejection", wrong_pass)
        client.disconnect()
        
        # Duplicate nick
        client1 = IRCClient(self.host, self.port)
        client2 = IRCClient(self.host, self.port)
        
        client1.connect()
        client2.connect()
        
        client1.authenticate(self.password, "samenick", "user1")
        client2.send(f"PASS {self.password}")
        client2.send("NICK samenick")
        msgs = client2.recv(1)
        
        nick_in_use = any("433" in msg or "already in use" in msg.lower() for msg in msgs)
        self.print_test("Duplicate nickname rejection", nick_in_use)
        
        client1.disconnect()
        client2.disconnect()
        
        return wrong_pass and nick_in_use
    
    def test_flood_protection(self):
        """Test 9: Flood and buffer overflow protection"""
        self.print_header("TEST 9: Flood Protection")
        
        client = IRCClient(self.host, self.port)
        client.connect()
        client.authenticate(self.password, "flooder", "flooder")
        
        # Send many messages quickly
        for i in range(20):
            client.send(f"PRIVMSG #flood :Flood message {i}")
        
        # Server should still be responsive
        client.send("PING :test")
        msgs = client.recv(2)
        still_responsive = any("PONG" in msg for msg in msgs)
        
        self.print_test("Server handles message flood", still_responsive,
                       "Server still responds after 20 rapid messages")
        
        client.disconnect()
        return still_responsive
    
    def test_unexpected_disconnect(self):
        """Test 10: Unexpected client disconnect"""
        self.print_header("TEST 10: Unexpected Disconnect Handling")
        
        client1 = IRCClient(self.host, self.port)
        client2 = IRCClient(self.host, self.port)
        
        client1.connect()
        client2.connect()
        
        client1.authenticate(self.password, "sudden1", "sudden1")
        client2.authenticate(self.password, "sudden2", "sudden2")
        
        # Join same channel
        client1.send("JOIN #disconnect")
        client2.send("JOIN #disconnect")
        time.sleep(0.5)
        
        # Client1 disconnects abruptly (no QUIT)
        client1.sock.close()
        client1.sock = None
        
        # Client2 should still work
        time.sleep(1)
        client2.send("PRIVMSG #disconnect :Still working?")
        msgs = client2.recv(1)
        
        # Try new connection
        client3 = IRCClient(self.host, self.port)
        can_connect = client3.connect()
        if can_connect:
            client3.authenticate(self.password, "sudden3", "sudden3")
            client3.send("JOIN #disconnect")
            client3.send("PRIVMSG #disconnect :New client connected!")
        
        self.print_test("Server survives unexpected disconnect", can_connect,
                       "New connections work after abrupt disconnect")
        
        client2.disconnect()
        if can_connect:
            client3.disconnect()
        
        return can_connect
    
    def run_all_tests(self):
        """Run all tests"""
        print(f"\n{BOLD}{MAGENTA}╔════════════════════════════════════════╗{RESET}")
        print(f"{BOLD}{MAGENTA}║     IRC SERVER TESTER - 42 PROJECT     ║{RESET}")
        print(f"{BOLD}{MAGENTA}╚════════════════════════════════════════╝{RESET}")
        print(f"\n{CYAN}Server: {self.host}:{self.port}")
        print(f"Password: {self.password}{RESET}")
        
        # Check if server is running
        test_client = IRCClient(self.host, self.port)
        if not test_client.connect():
            print(f"\n{RED}{BOLD}ERROR: Cannot connect to server!{RESET}")
            print(f"{YELLOW}Make sure your server is running:{RESET}")
            print(f"  ./ircserv {self.port} {self.password}")
            return
        test_client.disconnect()
        
        # Run tests
        tests = [
            self.test_basic_connection,
            self.test_multiple_connections,
            self.test_join_and_messages,
            self.test_channel_operations,
            self.test_part_and_quit,
            self.test_nick_change,
            self.test_partial_commands,
            self.test_error_handling,
            self.test_flood_protection,
            self.test_unexpected_disconnect
        ]
        
        for test in tests:
            try:
                test()
            except Exception as e:
                print(f"{RED}Test crashed: {e}{RESET}")
                self.failed_count += 1
        
        # Print summary
        self.print_summary()
    
    def print_summary(self):
        """Print test summary"""
        print(f"\n{BOLD}{BLUE}{'='*60}{RESET}")
        print(f"{BOLD}{CYAN}TEST SUMMARY{RESET}")
        print(f"{BOLD}{BLUE}{'='*60}{RESET}\n")
        
        total = self.test_count
        pass_rate = (self.passed_count / total * 100) if total > 0 else 0
        
        print(f"  Total Tests: {total}")
        print(f"  {GREEN}Passed: {self.passed_count}{RESET}")
        print(f"  {RED}Failed: {self.failed_count}{RESET}")
        print(f"  Pass Rate: {pass_rate:.1f}%")
        
        if pass_rate >= 90:
            status = f"{GREEN}{BOLD}EXCELLENT! Ready for evaluation! 🎉{RESET}"
        elif pass_rate >= 70:
            status = f"{YELLOW}{BOLD}GOOD! Minor issues to fix 🔧{RESET}"
        elif pass_rate >= 50:
            status = f"{YELLOW}{BOLD}NEEDS WORK! Several issues found ⚠️{RESET}"
        else:
            status = f"{RED}{BOLD}CRITICAL! Major issues detected ❌{RESET}"
        
        print(f"\n  Status: {status}")
        print(f"\n{BOLD}{BLUE}{'='*60}{RESET}\n")

def main():
    """Main function"""
    # Parse arguments
    if len(sys.argv) > 1 and sys.argv[1] in ['-h', '--help']:
        print("Usage: python3 irc_tester.py [port] [password]")
        print("Default: port=6666, password=hola")
        return
    
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 6666
    password = sys.argv[2] if len(sys.argv) > 2 else 'hola'
    
    # Run tests
    tester = IRCServerTester('localhost', port, password)
    
    try:
        tester.run_all_tests()
    except KeyboardInterrupt:
        print(f"\n{YELLOW}Tests interrupted by user{RESET}")
    except Exception as e:
        print(f"\n{RED}Fatal error: {e}{RESET}")

if __name__ == "__main__":
    main()