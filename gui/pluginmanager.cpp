#include "pluginmanager.h"
#include "parserdomainpolicy.h"

PluginManager::PluginManager(QObject *parent) :
    QObject(parent)
{
    requestHandler = new RequestHandler(this);
    loadPlugins();
}

void PluginManager::loadPlugins(void)
{
    QDir pluginDir(QCoreApplication::applicationDirPath());
    component_information c;
    if (!pluginDir.cd("plugins")) {
        QLOG_WARN() << "PluginManager :: " << "Plugin directory not found: " << pluginDir.path();
    }
    else {
        QLOG_INFO() << "PluginManager :: " << "Plugin Search path: " << pluginDir.path();

        foreach (QString fileName, pluginDir.entryList(QDir::Files))
        {
            QLOG_INFO() << "PluginManager :: " << "Checking file " << pluginDir.absoluteFilePath(fileName);
            QPluginLoader loader(pluginDir.absoluteFilePath(fileName));
            QString str;
            if (ParserPluginInterface* interface =
                    qobject_cast<ParserPluginInterface *>(loader.instance()))
            {
                if (interface->getInterfaceRevision() == _PARSER_PLUGIN_INTERFACE_REVISION) {
                    c.componentName = interface->getPluginName();
                    c.filename = fileName;
                    c.type = "plugin/parser";
                    c.version = interface->getVersion();

                    components.insert(QString("%1:%2").arg(c.type).arg(c.filename), c);
                    str.append(interface->getPluginName());
                    str.append(";;;");
                    str.append(fileName);
                    pluginList.append(str);
                    loadedPlugins.append(interface);
                    interface->initPlugin();
                    QList<QUrl> initURIs = interface->initialRequests();
                    for (int i=0; i<initURIs.count(); i++) {
                        requestHandler->request(initURIs.at(i),1);
                    }
                }
                else {

                    QLOG_WARN() << "PluginManager :: " << "Skipping plugin " << fileName << ", because it has the wrong interface revision";
                }
            }
            else {
                 QLOG_WARN() << "PluginManager :: " << "Could not load plugin " << loader.errorString();
            }
        }
        QLOG_TRACE() << "PluginManager :: " << "PluginList" << pluginList;
    }
}

ParserPluginInterface* PluginManager::getParser(QUrl url, bool* ok) {
    if (ok)
        *ok = false;
    for (int i = 0; i < loadedPlugins.count(); ++i) {
        ParserPluginInterface* parser = loadedPlugins.at(i);
        if (ParserDomainPolicy::supportsUrl(url, parser->getDomain())) {
            if (ok)
                *ok = true;
            return parser;
        }
    }
    return 0;
}

ParserPluginInterface* PluginManager::getPlugin(int i) {
    ParserPluginInterface* ret;

    ret = 0;

    if (i<loadedPlugins.count()) {
        ret = loadedPlugins.at(i);
    }

    return ret;
}

ParserPluginInterface* PluginManager::getPlugin(QString component) {
    ParserPluginInterface* ret;
    component_information c;

    ret = 0;

    c = components.value(component);

    for (int i=0; i<pluginList.count(); i++) {
        if (pluginList.at(i).endsWith(QString(";;;%1").arg(c.filename))) {
            ret = getPlugin(i);
            break;
        }
    }

    return ret;
}


QStringList PluginManager::getAvailablePlugins() {
    return components.keys();
}

component_information PluginManager::getInfo(QString name) {
    component_information ret;

    ret = components.value(name);

    return ret;
}

QStringList PluginManager::getSupportedDomains() {
    QStringList ret;

    for (int i=0; i<loadedPlugins.count(); i++) {
        ret << loadedPlugins.at(i)->getDomain();
    }

    return ret;
}

bool PluginManager::isSupported(QString domain) {
    const QUrl url = QUrl::fromUserInput(domain.trimmed());
    for (int i = 0; i < loadedPlugins.count(); ++i) {
        if (ParserDomainPolicy::supportsUrl(url, loadedPlugins.at(i)->getDomain()))
            return true;
    }
    return false;
}
