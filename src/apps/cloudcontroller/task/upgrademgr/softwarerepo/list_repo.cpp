#include <QSharedPointer>
#include <QTcpSocket>
#include <QDataStream>
#include <QStringList>
#include <QThread>
#include <iostream>

#include "list_repo.h"
#include "shell/abstract_task_container.h"
#include "corelib/network/rpc/api_invoker.h"
#include "shell/task_runner_worker.h"


namespace cloudcontroller{
namespace task{
namespace upgrademgr{
namespace softwarerepo{

using sn::corelib::network::ApiInvoker;
using cclib::shell::TaskRunnerWorker;

void ls_software_repo_callback(const ApiInvokeResponse &response, void *args)
{
   ListRepo *self = (ListRepo*)args;
   if(!response.getStatus()){
      self->processErrorResponse(response);
      self->getEventLoop().exit();
   }
   QDataStream in(response.getExtraData());
   QStringList files;
   in >> files;
   QStringList::const_iterator it = files.cbegin();
   while(it != files.cend()){
      std::cout << (*it).toStdString() << std::endl;
      it++;
   }
   self->getEventLoop().exit();
}

ListRepo::ListRepo(AbstractTaskContainer *taskContainer, const cclib::shell::TaskMeta &meta)
   : AbstractNetTask(taskContainer, meta)
{
}

void ListRepo::run()
{
   QSharedPointer<ApiInvoker>& apiInvoker = getApiInvoker();
   ApiInvokeRequest request("Repo/Info", "lsSoftwareRepoDir");
   apiInvoker->request(request, ls_software_repo_callback, (void*)this);
   m_eventLoop.exec();
}

}//softwarerepo
}//upgrademgr
}//task
}//cloudcontroller