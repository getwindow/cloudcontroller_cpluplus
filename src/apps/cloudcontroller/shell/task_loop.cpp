#include <QString>
#include <QByteArray>
#include <QPair>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>

#include <cstdio>
#include <iostream>
#include <sys/select.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>


#include "task_loop.h"
#include "io/terminal.h"
#include "kernel/errorinfo.h"

namespace cloudcontroller{
namespace shell{

using sn::corelib::Terminal;
using sn::corelib::TerminalColor;
using sn::corelib::ErrorInfo;

const int ASCII_NUL = 0;
const int ASCII_ESC = 27;

TaskLoop::TaskLoop()
{
   QPair<int, int> winSize = Terminal::getWindowSize();
   m_windowWidth = winSize.first;
   m_windowHeight = winSize.second;
   setupTerminalAttr();
}

void TaskLoop::setupTerminalAttr()
{
   tcgetattr(STDIN_FILENO, &m_savedTerminalAttr);
   struct termios terminalAttr;
   tcgetattr(STDIN_FILENO,&terminalAttr);
   terminalAttr.c_lflag &= ~ICANON;
   terminalAttr.c_lflag &= ~(ECHO);
   terminalAttr.c_cc[VMIN] = 1;
   terminalAttr.c_cc[VTIME] = 0;
   int status = tcsetattr(STDIN_FILENO, TCSANOW, &terminalAttr);
   Q_ASSERT_X(status == 0, "TaskLoop::setupTerminalAttr()", "tcsetattr error");
  
}

void TaskLoop::run()
{
   QString command;
   Terminal::writeText("welcome to use cloud controller system\n", TerminalColor::Cyan);
   Terminal::writeText(m_ps.toLocal8Bit(), TerminalColor::LightBlue);
   saveCycleBeginCursorPos();
   readCommand(command);
   while(command != "quit"){
      Terminal::writeText(m_ps.toLocal8Bit(), TerminalColor::LightBlue);
      saveCycleBeginCursorPos();
      readCommand(command);
   }
}

void TaskLoop::updateTerminalWindowSize(int width, int height)
{
   m_windowWidth = width;
   m_windowHeight = height;
   refreshLine();
}

void TaskLoop::readCommand(QString &command)
{
   fd_set readFds;
   char buf[32];
   memset(buf, ASCII_NUL, 32);
   int nfds = 1;
   FD_ZERO(&readFds);
   FD_SET(STDIN_FILENO, &readFds);
LABEL_AGAIN:
   int ret = select(nfds, &readFds, NULL, NULL, NULL);
   if(-1 == ret){
      if(errno == EINTR){
         goto LABEL_AGAIN;
      }
      throw ErrorInfo("select error");
   }
   for(int fd = 0; fd < nfds; fd++){
      if(FD_ISSET(fd, &readFds)){
         read(STDIN_FILENO, &buf, 31);
         QByteArray unit;
         bool escReaded = false;
         for(int i =0; i < 32; i++){
            if(buf[i] == ASCII_ESC && !escReaded){
               escReaded = true;
            }else if((buf[i] == ASCII_ESC && escReaded) || buf[i] == ASCII_NUL){
               break;
            }
            unit.append(buf[i]);
         }
         memset(buf, ASCII_NUL, 32);
         SpecialKeyName keyType = getKeyTypeName(unit);
         readUnitCycle(unit, keyType);
         if(keyType == SpecialKeyName::ASCII_CODE && unit == "\n"){
            command = m_cmd_buff.trimmed();
            m_cmd_buff.clear();
            std::cout << "\n";
            return;
         }
         goto LABEL_AGAIN;
      }
   }
}

void TaskLoop::readUnitCycle(QByteArray &unit, SpecialKeyName keyType)
{
   switch(keyType){
   case SpecialKeyName::ARROW_LEFT:
   case SpecialKeyName::ARROW_RIGHT:
   case SpecialKeyName::ARROW_UP:
   case SpecialKeyName::ARROW_DOWN:
   {
      arrowCommand(unit, keyType);
      break;
   }
   case SpecialKeyName::ASCII_CODE:
   {
      asciiCommand(unit, keyType);
      break;
   }
   default:
   {
      break;   
   }
   }
}

void TaskLoop::arrowCommand(QByteArray& unit, SpecialKeyName keyType)
{
   QPair<int, int> curPos = 
   if(keyType == SpecialKeyName::ARROW_LEFT){
      
   }else if(keyType == SpecialKeyName::ARROW_RIGHT){
      if(m_cursorX < m_ps.size()+m_cmd_buff.size() + 1){
         m_cursorX++;
      }
   }
   Terminal::setCursorPos(m_cursorX, m_cursorY);
}

void TaskLoop::asciiCommand(QByteArray &unit, SpecialKeyName)
{
//   if(unit != "\n"){
//      m_cmd_buff.insert(calculateInsertPos(), QString(unit));
//      m_cursorX += unit.size();
//      Terminal::forwardCursor(unit.size());
//      if(m_cursorX > m_windowWidth){
//         m_cursorY++;
//         m_cursorX = m_cursorX % m_windowWidth;
//         refreshLine(true);
//         return;
//      }
//   }
//   refreshLine();
}

void TaskLoop::refreshLine(bool newLine)
{
//   if(!newLine){
//      std::cout << "\x1b[2K" << std::flush;
//      Terminal::setCursorPos(0, m_cursorY);
//      Terminal::writeText(m_ps.toLocal8Bit(), TerminalColor::LightBlue);
//      std::cout << m_cmd_buff.toStdString() << std::flush;
//      Terminal::setCursorPos(m_cursorX, m_cursorY);
//   }else{
//      Terminal::setCursorPos(0, m_cursorY - 1);
//      std::cout << "\x1b[2K" << std::flush;
//      Terminal::writeText(m_ps.toLocal8Bit(), TerminalColor::LightBlue);
//      std::cout << m_cmd_buff.toStdString() << std::flush;
//      Terminal::setCursorPos(m_cursorX, m_cursorY);
//      updateCursorData();
//   }
}

int TaskLoop::calculateInsertPos()
{
   return m_cycleBeginX - (m_ps.size()+1);
}

TaskLoop::SpecialKeyName TaskLoop::getKeyTypeName(QByteArray& unit)
{
   SpecialKeyName keyType = SpecialKeyName::UNKNOW;
   if(unit.startsWith(ASCII_ESC) && unit.size() > 1){
      if(m_specialKeyMap.contains(unit)){
         keyType = m_specialKeyMap[unit];
      }
   }else{
      keyType = SpecialKeyName::ASCII_CODE;
   }
   return keyType;
}


QPair<int, int> TaskLoop::getCursorPos()
{
   char buf[32] = "\033[6n";
   write(STDOUT_FILENO, buf, sizeof(buf));
   memset(buf, 0, sizeof(buf));
   read (STDIN_FILENO ,buf ,sizeof(buf));
   QString ret(buf);
   QRegularExpression regex("\\[(?P<y>\\d+);(?P<x>\\d+)");
   QRegularExpressionMatch match = regex.match(ret);
   QPair<int, int> pos;
   if(match.hasMatch()){
      pos.first = match.captured("x").toInt();
      pos.second = match.captured("y").toInt();
   }
   return pos;
}

void TaskLoop::saveCycleBeginCursorPos()
{
   QPair<int, int> pair = getCursorPos();
   m_cycleBeginX = pair.first;
   m_cycleBeginY = pair.second;
}

TaskLoop::~TaskLoop()
{
   tcsetattr(STDIN_FILENO, TCSANOW, &m_savedTerminalAttr);
}

QMap<QByteArray, TaskLoop::SpecialKeyName> TaskLoop::m_specialKeyMap{
   {"\x1B[H", TaskLoop::SpecialKeyName::HOME},
   {"\x1B[5~", TaskLoop::SpecialKeyName::PAGE_UP},
   {"\x1B[6~", TaskLoop::SpecialKeyName::PAGE_DOWN},
   {"\x1B[3~", TaskLoop::SpecialKeyName::DEL},
   {"\x1B[F", TaskLoop::SpecialKeyName::END},
   {"\x1B[A", TaskLoop::SpecialKeyName::ARROW_UP},
   {"\x1B[B", TaskLoop::SpecialKeyName::ARROW_DOWN},
   {"\x1B[C", TaskLoop::SpecialKeyName::ARROW_RIGHT},
   {"\x1B[D", TaskLoop::SpecialKeyName::ARROW_LEFT},
};

}//shell
}//cloudcontroller