#include <ngx_core.h>
#include <ngx_http.h>
#include <assert.h>
#include <ngx_log.h>
#include "ngx_config.h"

static char * ngx_http_echo_post_parameter(ngx_conf_t *cf,ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_echo_post_parameter_handler(ngx_http_request_t *r);

static ngx_command_t ngx_http_echo_post_parameter_commands[] = {
    {
        ngx_string("echo_post_parameter"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF|NGX_CONF_NOARGS,
        ngx_http_echo_post_parameter,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

static char * ngx_http_echo_post_parameter(ngx_conf_t *cf,ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf,ngx_http_core_module);
    clcf->handler = ngx_http_echo_post_parameter_handler;

    return NGX_CONF_OK;
}

static ngx_http_module_t ngx_http_echo_post_parameter_module_ctx = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

ngx_module_t ngx_http_echo_post_parameter_module = {
    NGX_MODULE_V1,
    &ngx_http_echo_post_parameter_module_ctx,
    ngx_http_echo_post_parameter_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static void  ngx_http_read_post_body(ngx_http_request_t *r)
{
    size_t len = 0;
    r->headers_out.status = NGX_HTTP_OK;
    //设置响应报文数据类型和请求报文一样
    r->headers_out.content_type.data = ngx_palloc(r->pool,r->headers_in.content_type->value.len);
    memcpy(r->headers_out.content_type.data,r->headers_in.content_type->value.data,r->headers_in.content_type->value.len);
    r->headers_out.content_type.len = r->headers_in.content_type->value.len;
    
    ngx_chain_t *bufs = r->request_body->bufs;
    //获取请求报文内容长度
    while(bufs)
    {
         ngx_buf_t *buf = bufs->buf;
         len += (buf->last - buf->pos);
         bufs = bufs->next;
    }
    
    ngx_buf_t *response_body = ngx_create_temp_buf(r->pool,len);
    if (NULL == response_body)
    {
        ngx_log_error(NGX_LOG_ERR,r->connection->log,0,"ngx_create_temp_buf return null!");
        return;
    }

    bufs = r->request_body->bufs;
    len = 0;
    //复制请求报文内容到响应报文
    while(bufs)
    {
         ngx_buf_t *buf = bufs->buf;
         memcpy(response_body->pos+len,buf->pos,buf->last - buf->pos);
         len += (buf->last - buf->pos);
         bufs = bufs->next;
    }
    
    int rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        ngx_log_error(NGX_LOG_ERR,r->connection->log,0,"ngx_http_send_header err:%d",rc);
        return;
    }
    
    response_body->last = response_body->pos + len;
    response_body->last_buf = 1;

    ngx_chain_t output;
    output.buf = response_body;
    output.next = NULL;
    
    ngx_http_output_filter(r,&output);
    
    return;
}

static ngx_int_t ngx_http_echo_post_parameter_handler(ngx_http_request_t *r)
{
    if (!(r->method&NGX_HTTP_POST))//只处理POST请求
    {
        return NGX_HTTP_NOT_ALLOWED;
    }
    
    ngx_int_t rc = ngx_http_read_client_request_body(r,ngx_http_read_post_body);//设置读取完请求体后的回调处理函数

    if (rc == NGX_ERROR || rc >= NGX_HTTP_SPECIAL_RESPONSE) {
            return rc;
        }
     return NGX_OK;
}
