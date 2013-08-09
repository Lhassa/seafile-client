#include <cstdio>
#include <iostream>
#include <QNetworkAccessManager>

#include "api-client.h"

#include <jansson.h>

namespace {

const char *kApiLoginUrl = "/api2/auth-token/";
const char *kContentTypeForm = "application/x-www-form-urlencoded";

} // namespace


SeafileApiClient::SeafileApiClient() {
    network_access_mgr_ = new QNetworkAccessManager(this);
}

SeafileApiClient* SeafileApiClient::singleton_ = NULL;

SeafileApiClient* SeafileApiClient::instance()
{
    if (singleton_ == 0) {
        singleton_ = new SeafileApiClient();
    }

    return singleton_;
}

void SeafileApiClient::accountLogin(const QUrl& serverUrl,
                                  const QString& username,
                                  const QString& password)
{
    QUrl url(serverUrl);
    url.setPath(url.path() + kApiLoginUrl);

    qDebug("Login to %s\n", url.toString().toUtf8().data());
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, kContentTypeForm);
    // form post params
    QUrl params;
    params.addQueryItem("username", username);
    params.addQueryItem("password", password);

    current_login_account_.serverUrl = serverUrl;
    current_login_account_.username = username;

    if (current_login_request_) {
        delete current_login_request_;
    }

    current_login_request_ = network_access_mgr_->post(request, params.encodedQuery());
    // ignore ssl error
    current_login_request_->ignoreSslErrors();

    connect(current_login_request_, SIGNAL(finished()), this, SLOT(loginRequestFinished()));
}

void SeafileApiClient::loginRequestFinished()
{
    if (current_login_request_->error() != QNetworkReply::NoError) {
        qDebug("failed to send login request:%s\n",
                 current_login_request_->errorString().toUtf8().data());
        emit accountLoginFailed();
        return;
    }

    int code = current_login_request_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (code != 200) {
        qDebug("failed to login: status code %d\n", code);
        emit accountLoginFailed();
    }

    QByteArray raw = current_login_request_->readAll();
    json_error_t error;
    json_t *root = json_loads(raw.data(), 0, &error);
    if (!root) {
        qDebug("failed to parse json:%s\n", error.text);
        emit accountLoginFailed();
        return;
    }

    const char *token = json_string_value(json_object_get (root, "token"));
    if (token == NULL) {
        qDebug("failed to parse json:%s\n", error.text);
        emit accountLoginFailed();
    }

    qDebug("login successful, token is %s\n", token);
    current_login_account_.token = token;
    emit accountLoginSuccess(current_login_account_);
}

void SeafileApiClient::getAccountRepos(const Account& account)
{
}