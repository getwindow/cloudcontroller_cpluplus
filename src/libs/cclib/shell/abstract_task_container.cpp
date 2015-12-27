#include <QRegularExpression>
#include <QScopedPointer>

#include "corelib/kernel/errorinfo.h"
#include "corelib/command/route_item.h"
#include "corelib/command/route_match_result.h"

#include "abstract_task_container.h"
#include "task_meta.h"
#include "abstract_task.h"
#include "abstract_task_loop.h"

namespace cclib{
namespace shell{

using sn::corelib::RouteItem;
using sn::corelib::RouteMatchResult;
using sn::corelib::Terminal;
using sn::corelib::ErrorInfo;

AbstractTaskContainer::AbstractTaskContainer(const QString& name, AbstractTaskLoop &loop)
   : m_name(name),
     m_taskLoop(loop),
     m_app(*Application::instance())
{}

const QString& AbstractTaskContainer::getName()
{
   return m_name;
}

AbstractTaskLoop& AbstractTaskContainer::getTaskLoop()
{
   return m_taskLoop;
}

void AbstractTaskContainer::run(const QString& command)
{
   TaskMeta meta;
   RouteMatchResult routeMatch = m_router.match(command.split(QRegularExpression("\\s+")));
   if(!routeMatch.getStatus()){
      Terminal::writeText("invalid command\n", TerminalColor::Red);
      printUsage();
      throw ErrorInfo();
   }
   meta.setContainer(m_name);
   meta.setCategory(routeMatch.getParam("category"));
   meta.setName(routeMatch.getParam("name"));
   meta.setTaskArgs(routeMatch.getParams());
   try{
      runTask(meta);
   }catch(ErrorInfo errorInfo){
      QString str(errorInfo.toString());
      if(str.size() > 0){
         str += '\n';
         Terminal::writeText(str.toLocal8Bit(), TerminalColor::Red);
      }
   }
}

void AbstractTaskContainer::runTask(const TaskMeta &meta)
{
   QString key(meta.getContainer()+ '_' +meta.getCategory() + '_' + meta.getName());
   Q_ASSERT_X(m_taskRegisterPool.contains(key), "AbstractTaskContainer::run()", QString("command : %1 is not exist").arg(key).toLatin1());
   AbstractTask* (*initializer)(AbstractTaskContainer*, const TaskMeta&) = m_taskRegisterPool[key];
   QScopedPointer<AbstractTask> task(initializer(this, meta));
   task->run();
}

void AbstractTaskContainer::printUsage()const
{
   QListIterator<UsageTextItemType> iterator(m_usageTextPool);
   while(iterator.hasNext()){
      UsageTextItemType item(iterator.next());
      Terminal::writeText(item.first.toLocal8Bit(), item.second);
   }
}

void AbstractTaskContainer::addUsageText(const QString& text, TerminalColor color)
{
   m_usageTextPool.append({
                             {text, color}                            
                          });
}

void AbstractTaskContainer::writeSubMsg(const QString &msg)
{
   QString toBeWrite(msg);
   toBeWrite += "\n";
   Terminal::writeText("> ", TerminalColor::Green);
   Terminal::writeText(toBeWrite.toLocal8Bit(), TerminalColor::Default);
}

void AbstractTaskContainer::addTaskRoute(const QString& name, const QString& route, int priority, const QMap<QString, QString>& defaultParams)
{
   Q_ASSERT_X(defaultParams.contains("category") && defaultParams.contains("name"), 
              "AbstractTaskContainer::addTaskRoute()", "must contain key category and key name");
   m_router.addRoute(name, RouteItem(route, defaultParams), priority);
}

void AbstractTaskContainer::loadHandler(const QMap<QString, QString>&)
{
   if(!m_containerPs.isEmpty()){
      m_psBackup = m_taskLoop.getConsolePsText();
      m_taskLoop.setConsolePsText(m_containerPs);
   }
}

void AbstractTaskContainer::unloadHandler()
{
   if(!m_containerPs.isEmpty() && !m_psBackup.isEmpty()){
      m_taskLoop.setConsolePsText(m_psBackup);
   }
}

AbstractTaskContainer::~AbstractTaskContainer()
{}

}//shell
}//cclib