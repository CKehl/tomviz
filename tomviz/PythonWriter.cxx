/******************************************************************************

  This source file is part of the tomviz project.

  Copyright Kitware, Inc.

  This source code is released under the New BSD License, (the "License").

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

******************************************************************************/
#include "PythonWriter.h"

#include "DataSource.h"

#include <vtkImageData.h>
#include <vtkTrivialProducer.h>

#include <QDebug>

namespace tomviz {

PythonWriter::PythonWriter(Python::Object instance) : m_instance(instance)
{
}

bool PythonWriter::write(QString fileName, vtkImageData* data)
{
  Python python;
  auto module = python.import("tomviz.io._internal");
  if (!module.isValid()) {
    qCritical() << "Failed to import tomviz.io._internal module.";
    return false;
  }
  auto writerFunction = module.findFunction("execute_writer");
  if (!writerFunction.isValid()) {
    qCritical() << "Failed to import tomviz.io._internal.execute_writer";
    return false;
  }

  Python::Tuple args(3);
  Python::Object argInstance = m_instance;
  args.set(0, argInstance);
  Python::Object fileArg(fileName);
  args.set(1, fileArg);

  Python::Object imageArg = Python::VTK::GetObjectFromPointer(data);
  args.set(2, imageArg);
  auto res = writerFunction.call(args);

  if (!res.isValid()) {
    qCritical("Error calling the writer");
    return false;
  }

  return true;
}

PythonWriterFactory::PythonWriterFactory(QString description,
                                         QStringList extensions,
                                         Python::Object cls)
  : m_description(description), m_extensions(extensions), m_class(cls)
{
}

QString PythonWriterFactory::getDescription() const
{
  return m_description;
}

QStringList PythonWriterFactory::getExtensions() const
{
  return m_extensions;
}

QString PythonWriterFactory::getFileDialogFilter() const
{
  QStringList filterExt;
  foreach (auto ext, m_extensions) {
    filterExt << QString("*.%1").arg(ext);
  }
  QString filter =
    QString("%1 (%2)").arg(m_description).arg(filterExt.join(QString(" ")));

  return filter;
}

PythonWriter PythonWriterFactory::createWriter() const
{
  Python python;
  auto module = python.import("tomviz.io._internal");
  if (!module.isValid()) {
    qCritical() << "Failed to import tomviz.io._internal module.";
    return Python::Object();
  }
  auto factory = module.findFunction("create_writer_instance");
  if (!factory.isValid()) {
    qCritical() << "Failed to import tomviz.io_internal.create_writer_instance";
    return Python::Object();
  }

  Python::Tuple args(1);
  Python::Object argClass = m_class;
  args.set(0, argClass);
  auto res = factory.call(args);
  if (!res.isValid()) {
    qCritical("Error calling create_writer_instance.");
    return Python::Object();
  }
  return PythonWriter(res);
}
}
