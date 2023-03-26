using System;
using System.Threading.Tasks;
using Amazon.S3;
using Amazon.S3.Model;
using Amazon.Lambda.Core;
using Amazon.Lambda.APIGatewayEvents;

[assembly: LambdaSerializer(typeof(Amazon.Lambda.Serialization.Json.JsonSerializer))]

namespace OpenLyricsMetrics
{
    public class MetricsServer
    {
        private AmazonS3Client s3 = new();

        public async Task HandleRequest(APIGatewayHttpApiV2ProxyRequest evt, ILambdaContext ctx)
        {
            if(string.IsNullOrEmpty(evt.Body))
            {
                return;
            }

            PutObjectRequest putRequest = new();
            putRequest.BucketName = "foo-openlyrics-metrics";
            putRequest.Key = Guid.NewGuid().ToString();
            putRequest.ContentBody = evt.Body;
            PutObjectResponse resp = await s3.PutObjectAsync(putRequest);
            ctx.Logger.LogInformation($"Completed writing {evt.Body.Length} characters to S3 with result: {resp.HttpStatusCode}");
        }
    }
}
